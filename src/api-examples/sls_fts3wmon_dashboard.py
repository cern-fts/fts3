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
import tempfile

import xml.etree.ElementTree as ET

SLS_NS = 'http://sls.cern.ch/SLS/XML/update'


def _add_numeric(e_data, name, value):
    e_numeric = ET.SubElement(e_data, '{%s}numericvalue' % SLS_NS, {'name': name})
    e_numeric.text = str(value)


class Fts3SlsPlus(object):
    """
    Enriched Service Level Status
    """

    VOS = ['atlas', 'cms', 'lhcb']

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
        io = tempfile.TemporaryFile()
        self.curl.setopt(pycurl.WRITEDATA, io)
        self.curl.setopt(pycurl.URL, url)
        self.curl.perform()
        io.seek(0)
        return io.read()

    def __init__(self, fts3_name, sls_url, dashb_base, interval):
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
        self.interval = interval

    def generate_sls(self):
        """
        Generate the enriched SLS
        """
        sls = ET.fromstring(self._get(self.sls_url))
        data_elm = sls.find('{%s}data' % SLS_NS)
        if data_elm is None:
            raise RuntimeError('Could not find the data node')

        for vo in self.VOS:
            dashb = json.loads(self._get(
                self.dashb_base + '/transfer-matrix.json?vo=%s&server=%s&interval=%d' %
                (vo, self.fts3_name, self.interval)
            ))

            bytes_key = dashb['transfers']['key']['bytes_xs']
            transfers_key = dashb['transfers']['key']['files_xs']
            errors_key = dashb['transfers']['key']['errors_xs']

            rows = dashb['transfers']['rows']
            n_transfers = 0
            n_errors = 0
            throughput = 0
            if rows:
                row = rows[0]
                transferred = row[bytes_key]
                n_transfers = row[transfers_key]
                n_errors = row[errors_key]

                throughput = transferred / (self.interval * 60)

            _add_numeric(data_elm, '%s_finished' % vo, n_transfers)
            _add_numeric(data_elm, '%s_errors' % vo, n_errors)
            _add_numeric(data_elm, '%s_throughput' % vo, throughput)

        ET._namespace_map[SLS_NS] = ''
        return ET.tostring(sls).replace('<:', '<').replace('</:', '</').replace(':=', '=')

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
    parser.add_option(
        '--interval', help='Interval in minutes', type=int,
        default=60
    )
    options, args = parser.parse_args()

    sls = Fts3SlsPlus(options.fts, options.sls, options.dashboard, options.interval)
    print sls
