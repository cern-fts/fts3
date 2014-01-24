#!/usr/bin/env python
import json
from common import get_url
from optparse import OptionParser

def get_slow_pairs(threshold = 1, vo = None):
    content = get_url('https://fts3-pilot.cern.ch:8449/fts3/ftsmon/overview', vo = vo, page = 'all')
    pairs = json.loads(content)
    
    slow = []
    for pair in pairs['items']:
        if 'current' in pair and pair['current'] < threshold:
            slow.append(pair)
        
    return slow
    

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option('-v', '--vo', dest = 'vo', help = 'Query only for a given VO', default = None)
    parser.add_option('-t', '--threshold', dest = 'threshold', help = 'Threshold in MB', default = 1, type = 'float')

    (options, args) = parser.parse_args()
    
    slow = get_slow_pairs(options.threshold, options.vo)
    for pair in slow:
        print "%(source_se)s => %(dest_se)s with throughput %(current).2f" % pair
