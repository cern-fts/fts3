#!/usr/bin/env python
import json
from common import get_url
from optparse import OptionParser

def get_servers():
    servers = get_url('https://fts3-pilot-mon.cern.ch/ftsmon/stats/servers')
    return json.loads(servers)

if __name__ == '__main__':
    servers = get_servers()
    for server in servers:
        print server['hostname']
        print "\tActive:", server.get('active', 0)
        print "\tReceived:", server.get('submissions', 0)
        print "\tExecuted:", server.get('transfers', 0)
