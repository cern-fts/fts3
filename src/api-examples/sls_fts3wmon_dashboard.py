#!/usr/bin/env python
# Copyright notice:
# Copyright (C) CERN, 2015.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import json
import pycurl
import optparse
import StringIO

import xml.etree.ElementTree as ET

SLS_NS = 'http://sls.cern.ch/SLS/XML/update'


def _add_numeric(e_data, name, value):
    e_numeric = ET.SubElement(e_data, '{%s}numericvalue' % SLS_NS, {'name': name})
    e_numeric.text = str(value)


class Fts3SlsPlus(object):
    """
    Enriched Service Level Status
    """

    INTERVAL = 60  # In minutes

    @staticmethod
    def _setup_curl():
        """
        Setup a curl handle
        """
        curl = pycurl.Curl()
        curl.setopt(pycurl.FOLLOWLOCATION, True)
        curl.setopt(pycurl.SSL_VERIFYHOST, False)
        curl.setopt(pycurl.SSL_VERIFYPEER, False)
        return curl

    def _get(self, url):
        """
        Do an HTTP GET
        """
        io = StringIO.StringIO()
        self.curl.setopt(pycurl.WRITEDATA, io)
        self.curl.setopt(pycurl.URL, url)
        self.curl.perform()
        return io.getvalue()

    def __init__(self, fts3_name, sls_url, dashb_base):
        """
        Constructor
        :param fts3_name:  The FTS3 name as sent to the dashboard
        :param sls_url:    The SLS url used as base
        :param dashb_base: Dashboard base url
        """
        self.curl = self._setup_curl()
        self.fts3_name = fts3_name
        self.sls_url = sls_url
        self.dashb_base = dashb_base

    def generate_sls(self):
        """
        Generate the enriched SLS
        """
        sls = ET.fromstring(self._get(self.sls_url))
        dashb = json.loads(self._get(
            self.dashb_base + '/transfer-ranking?interval=%d&vo=atlas&vo=cms&vo=lhcb&server=%s&grouping=vo' %
            (self.INTERVAL, self.fts3_name)
        ))

        # Insert new data
        data_elm = sls.find('{%s}data' % SLS_NS)
        if data_elm is None:
            raise RuntimeError('Could not find the data node')

        for entry in dashb['transfers-vo']:
            vo = entry['name']
            n_transfers = entry['bins'][0]['files_xs']
            n_errors = entry['bins'][0]['errors_xs']
            transefered_bytes = entry['bins'][0]['bytes_xs']
            transfer_time = entry['bins'][0]['transfer_time']
            throughput = transefered_bytes / float(transfer_time)

            _add_numeric(data_elm, '%s_finished' % vo, n_transfers)
            _add_numeric(data_elm, '%s_errors' % vo, n_errors)
            _add_numeric(data_elm, '%s_throughput' % vo, throughput)

        return ET.tostring(sls)

    def __str__(self):
        """
        Convenience shortcut
        """
        return self.generate_sls()


if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option(
        '--fts', help='FTS3 endpoint name',
        default='fts3.cern.ch'
    )
    parser.add_option(
        '--sls', help='FTS3 SLS content',
        default='https://fts3.cern.ch:8449/fts3/ftsmon/stats/servers?format=sls'
    )
    parser.add_option(
        '--dashboard', help='FTS3 dashboard',
        default='http://dashb-fts-transfers.cern.ch/dashboard/request.py'
    )
    options, args = parser.parse_args()

    sls = Fts3SlsPlus(options.fts, options.sls, options.dashboard)
    print sls
