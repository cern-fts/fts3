#!/usr/bin/env python
#
# Copyright (c) CERN 2015
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import json
import logging
import optparse
import os
import pycurl
import sys
import urlparse
import urllib

from StringIO import StringIO


log = logging.getLogger(__name__)

_PYCURL_SSL = pycurl.version_info()[5].split('/')[0]


class FtsOracle(object):
    """
    Does predictions of behavior looking at the FTS3 web mon
    """

    def _setup_curl(self):
        """
        Setup curl handle
        """
        self.curl = pycurl.Curl()
        self.curl.setopt(pycurl.SSLCERT, self.ucert)
        self.curl.setopt(pycurl.SSLKEY, self.ukey)

        if _PYCURL_SSL == 'NSS':
            self.curl.setopt(pycurl.CAINFO, self.ucert)
        self.curl.setopt(pycurl.CAPATH, '/etc/grid-security/certificates')

    def _get_json(self, path, **kwargs):
        """
        Ask for the json response for the path with **kwargs query parameters
        """
        url_unparsed = (
            self.endpoint_parsed.scheme,
            self.endpoint_parsed.netloc,
            self.endpoint_parsed.path + '/' + path,
            None,
            urllib.urlencode(kwargs),
            None
        )

        response = StringIO()

        url = urlparse.urlunparse(url_unparsed)
        log.debug('GET %s' % url)
        self.curl.setopt(pycurl.URL, url)
        self.curl.setopt(pycurl.WRITEDATA, response)
        self.curl.perform()
        http_code = self.curl.getinfo(pycurl.HTTP_CODE)
        if http_code / 100 != 2:
            raise Exception('HTTP error %d' % http_code)

        body = response.getvalue()
        log.debug(body)
        return json.loads(body)

    def __init__(self, endpoint, ucert, ukey):
        """
        Constructor
        """
        self.endpoint = endpoint
        self.ucert = ucert
        self.ukey = ukey

        if not self.ucert:
            if 'X509_USER_CERT' in os.environ and 'X509_USER_KEY' in os.environ:
                self.ucert = os.environ['X509_USER_CERT']
                self.ukey = os.environ['X509_USER_KEY']
            else:
                self.ucert = self.ukey = '/tmp/x509up_u%d' % os.getuid()
        elif not self.ukey:
            self.ukey = self.ucert

        self._setup_curl()
        self.endpoint_parsed = urlparse.urlparse(endpoint)

    def summary(self, source, destination, vo=None):
        """
        Prints current status of the queue for the given pair
        """
        # Query overview
        overview = self._get_json('overview', source_se=source, dest_se=destination)
        queued = overview['summary']['submitted']
        active = overview['summary']['active']
        finished = overview['summary']['finished']
        failed = overview['summary']['failed']
        total = finished + failed
        if total > 0:
            rate = float(finished) / total
        else:
            rate = 0

        # Query optimizer
        optimizer = self._get_json('optimizer/detailed', source=source, destination=destination)
        try:
            optimizer_active = optimizer['evolution']['items'][0]['active']
        except (ValueError, KeyError, IndexError):
            optimizer_active = None
        fixed_limit = optimizer['fixed']

        # Figure out configuration
        config = self._get_json('config/server')
        max_per_link = None
        max_per_se = None
        for entry in config['per_vo']:
            if entry['vo_name'] == vo or (entry['vo_name'] in (None, '*') and max_per_link is None):
                max_per_link = entry['max_per_link']
                max_per_se = entry['max_per_se']

        limits = self._get_json('config/limits', page='all')
        hard_limit = None
        for limit in limits['items']:
            if (limit['source_se'] is None and limit['dest_se'] == destination) or \
               (limit['source_se'] == source and limit['dest_se'] is None) or \
               (limit['source_se'] == source and limit['dest_se'] == destination):
                hard_limit = limit['active']

        # Print what we have figured out
        actual_active = optimizer_active
        print 'Queued:           %d' % queued
        print 'Active:           %d' % active
        print 'Success rate:     %.2f%%' % (rate * 100)
        print 'Optimizer active: %s' % optimizer_active
        if fixed_limit:
            actual_active = fixed_limit
            print 'Active fixed to %d' % actual_active

        if hard_limit:
            print 'Max active limited by configuration to %d' % hard_limit
            if actual_active:
                actual_active = min(actual_active, hard_limit)
            else:
                actual_active = hard_limit
        if max_per_link:
            print 'Max active per link limited to %d ' % max_per_link
            print 'Max active per storage limited to %d' % max_per_se
            if actual_active:
                actual_active = min(actual_active, max_per_link, max_per_se)
            else:
                actual_active = min(max_per_link, max_per_se)
        print 'Current active top: %s' % actual_active

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('-s', '--endpoint', default='https://fts3-pilot.cern.ch:8449/fts3/ftsmon',
                      help='Web monitoring endpoint')
    parser.add_option('-E', '--cert', default=None,
                      help='User certificate')
    parser.add_option('-K', '--key', default=None,
                      help='User private key')
    parser.add_option('-d', '--debug', default=False, action='store_true',
                      help='Enable debug output')

    options, args = parser.parse_args()

    log_root = logging.getLogger()
    log_handle = logging.StreamHandler(sys.stderr)
    if options.debug:
        log_root.setLevel(logging.DEBUG)
        log_handle.setLevel(logging.DEBUG)
    else:
        log_root.setLevel(logging.INFO)
        log_handle.setLevel(logging.INFO)
    log_root.addHandler(log_handle)

    if len(args) < 2:
        parser.error('Need a source and a destination and an optional VO')

    FtsOracle(options.endpoint, options.cert, options.key).summary(*args)
