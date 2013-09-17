# Copyright notice:
# Copyright (C) Members of the EMI Collaboration, 2010.
#
# See www.eu-emi.eu for details on the copyright holders
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

from datetime import datetime, timedelta
from django.db.models import Q, Count, Avg
from ftsweb.models import Job, File, ConfigAudit, Host
from ftsweb.models import ProfilingSnapshot, ProfilingInfo
from jsonify import jsonify, jsonify_paged


STATES               = ['SUBMITTED', 'READY', 'ACTIVE', 'FAILED', 'FINISHED', 'CANCELED', 'STAGING', 'NOT_USED']
ACTIVE_STATES        = ['SUBMITTED', 'READY', 'ACTIVE', 'STAGING']
FILE_TERMINAL_STATES = ['FINISHED', 'FAILED', 'CANCELED']


@jsonify_paged
def configurationAudit(httpRequest):
    return ConfigAudit.objects.order_by('-datetime')



def _getCountPerState(age = None):
    count = {}
    
    query = File.objects
    if age:
        notBefore = datetime.utcnow() - age
        query = query.filter(Q(finish_time__gte = notBefore) | Q(finish_time__isnull = True))
    
    query = query.values('file_state').annotate(number = Count('file_state'))
    
    for row in query:
        count[row['file_state'].lower()] = row['number']
    
    for s in filter(lambda s: s not in count, map(lambda s: s.lower(), STATES)):
        count[s] = 0
    
    # Couple of aggregations
    count['queued'] = count['submitted'] + count['ready']
    count['total'] = count['finished'] + count['failed'] + count['canceled']
    
    return count



def _getTransferAndSubmissionPerHost(timewindow):
    hostnames = []
    
    notBefore = datetime.utcnow() - timewindow
        
    submissions = {}
    query = Job.objects.values('submit_host')\
                       .filter(submit_time__gte = notBefore)\
                       .annotate(count = Count('submit_host'))
    for j in query:
        submissions[j['submit_host']] = j['count']
        hostnames.append(j['submit_host'])
        
    transfers = {}
    query = File.objects.values('transferHost')\
                        .filter(Q(finish_time__gte = notBefore) | Q(finish_time__isnull = True))\
                        .annotate(count = Count('transferHost'))
    for t in query:
        # Submitted do not have a transfer host!
        if t['transferHost']:
            transfers[t['transferHost']] = t['count']
            if t['transferHost'] not in hostnames:
                hostnames.append(t['transferHost'])
                
    active = {}
    query = File.objects.filter(file_state = 'ACTIVE').values('transferHost').annotate(count = Count('transferHost'))
    for t in query:
        active[t['transferHost']] = t['count']
        
    servers = {}
    for h in hostnames:
        if h in submissions: s = submissions[h]
        else:                s = 0
        if h in transfers:   t = transfers[h]
        else:                t = 0
        if h in active:      a = active[h]
        else:                a = 0
        servers[h] = {'hostname': h, 'submissions': s, 'transfers': t, 'active': a}
        
    return servers



def _getStateCountPerVo(timewindow):
    perVo = {}
    query = File.objects.values('file_state', 'job__vo_name')\
                        .filter(Q(finish_time__gte = datetime.utcnow() - timewindow) | Q(finish_time__isnull = True))\
                        .annotate(count = Count('file_state'))\
                        .order_by('file_state')
    for voJob in query:
        vo = voJob['job__vo_name']
        if vo not in perVo:
            perVo[vo] = {}
        perVo[vo][voJob['file_state']] = voJob['count']

    return perVo



def _getAllPairs(notBefore, source = None, dest = None):
    pairs = []
    
    query = File.objects.filter(Q(finish_time__gte = notBefore) | Q(finish_time = None))\
                        .values('source_se', 'dest_se')
    if source:
        query = query.filter(source_se = source)
    if dest:
        query = query.filter(dest_se = dest)
    
    for pair in query.distinct():
        pairs.append((pair['source_se'], pair['dest_se']))
    return pairs



def _getAveragePerPair(pairs, notBefore):
    avg = {}
    
    pairsAvg = File.objects.exclude(file_state__in = ACTIVE_STATES).filter(finish_time__gte = notBefore)\
                              .values('source_se', 'dest_se', 'tx_duration', 'throughput')\
                              .annotate(Avg('tx_duration'), Avg('throughput'))

    for pair in pairsAvg:
        sePair = (pair['source_se'], pair['dest_se'])
        avg[sePair] = {'avgDuration': pair['tx_duration__avg'],
                       'avgThroughput': pair['throughput__avg']}
    
    return avg



def _getFilesInStatePerPair(pairs, states, notBefore):
    statesPerPair = {}
    for pair in pairs:
        statesPerPair[pair] = {}
    
    query = File.objects
    
    if states:
        query = query.filter(file_state__in = states)
        
    query = query.filter(Q(finish_time__gt = notBefore) | Q(finish_time__isnull = True))\
                        .values('source_se', 'dest_se', 'file_state')\
                        .annotate(count = Count('file_state'))

    for result in query:
        pair = (result['source_se'], result['dest_se'])
        if pair in pairs:
            statesPerPair[pair][result['file_state']] = result['count']

    
    return statesPerPair



def _getStatsPerPair(source_se, dest_se, timewindow):
    
    notBefore = datetime.utcnow() - timewindow
    
    allPairs      = _getAllPairs(notBefore, source_se, dest_se)
    avgsPerPair   = _getAveragePerPair(allPairs, notBefore)
    
    statesPerPair = _getFilesInStatePerPair(allPairs, FILE_TERMINAL_STATES, notBefore)
        
    pairs = []
    for pair in allPairs:
        p = {'source': pair[0], 'destination': pair[1]}
           
        if pair in statesPerPair:
            states = statesPerPair[pair]
                
            # Success rate
            terminal = dict((k, v) for k, v in states.items() if k in FILE_TERMINAL_STATES)
            total = float(reduce(lambda a,b: a+b, terminal.values(), 0))
            success = float(states['FINISHED'] if 'FINISHED' in states else 0)
            
            if len(terminal):
                p['successRate'] = (success/total) * 100
            else:
                p['successRate'] = None
        else:
            p['successRate'] = None
            
        if pair in avgsPerPair and p['successRate'] is not None:
            p.update(avgsPerPair[pair])
        else:
            p['avgDuration'] = None
            p['avgThroughput'] = None

        pairs.append(p)
        
    return sorted(pairs,
                  key = lambda p: (p['successRate'], p['avgThroughput']),
                  reverse = True)



def _getRetriedStats(timewindow):
    notBefore = datetime.utcnow() - timewindow
    
    retriedObjs = File.objects.filter(file_state__in = ['FAILED', 'FINISHED'], finish_time__gte = notBefore, retry__gt = 0)\
                          .values('file_state').annotate(number = Count('file_state'))
    retried = {}
    for f in retriedObjs:
        retried[f['file_state'].lower()] = f['number']
    for s in [s for s in ['failed', 'finished'] if s not in retried]:
        retried[s] = 0
    
    return retried    


@jsonify
def overview(httpRequest):
    overall = _getCountPerState(timedelta(hours = 24))
    lastHour = _getCountPerState(timedelta(hours = 1))
    retried = _getRetriedStats(timedelta(hours = 1))
        
    return {
       'lastday': overall,
       'lasthour': lastHour,
       'retried': retried
    }
    

def _getHostHeartBeatAndSegment():
    allHosts = Host.objects.order_by('hostname').filter(beat__gte = datetime.utcnow() - timedelta(minutes = 2))
    hostCount = len(allHosts)
    
    if hostCount == 0:
        return {}
    
    segmentSize = 0xFFFFFFFF / hostCount
    segmentMod  = 0xFFFFFFFF % hostCount
    
    hosts = {}
    index = 0
    for h in allHosts:
        hosts[h.hostname] = {
            'beat'    : h.beat,
            'index'   : index,
            'segment' : {
                'start': "%08X" % (segmentSize * index),
                'end'  : "%08X" % (segmentSize * (index + 1) - 1),
            }
        }
        
        if index == hostCount - 1:
            hosts[h.hostname]['segment']['end'] = "%08X" % (segmentSize * (index + 1) + segmentMod)
        
        index += 1       
    
    return hosts

@jsonify
def servers(httpRequest):
    beats     = _getHostHeartBeatAndSegment()
    transfers = _getTransferAndSubmissionPerHost(timedelta(hours = 12))
    
    servers = beats
    for k in servers:
        servers[k].update(transfers.get(k, {}))
    
    return servers



def _avgField(pairs, field):
    pairsWithTerminal = filter(lambda p: p['successRate'] is not None, pairs)
    nTerminalPairs = len(pairsWithTerminal)
    if nTerminalPairs:
        return reduce(lambda a, b: a + b, map(lambda p: p[field], pairsWithTerminal), 0) / nTerminalPairs
    else:
        return 0



def _sumStatus(stDictA, stDictB):
    r = {}
    keys = stDictA.keys() + stDictB.keys()
    for s in keys:
        r[s] = stDictA.get(s, 0) + stDictB.get(s, 0)
        
    return r


@jsonify
def pairs(httpRequest):
    source_se = httpRequest.GET['source_se'] if 'source_se' in httpRequest.GET else None  
    dest_se   = httpRequest.GET['dest_se'] if 'dest_se' in httpRequest.GET else None    
    pairs = _getStatsPerPair(source_se, dest_se, timedelta(minutes = 30))
    
    # Build aggregates
    aggregate = {}
    aggregate['successRate']   = _avgField(pairs, 'successRate')
    aggregate['avgThroughput'] = _avgField(pairs, 'avgThroughput')
    aggregate['avgDuration']   = _avgField(pairs, 'avgDuration')
    
    return {'pairs': pairs,
            'aggregate': aggregate
    }


@jsonify
def pervo(httpRequest):
    return _getStateCountPerVo(timedelta(minutes = 30))


@jsonify
def profiling(httpRequest):
    profiling = {}
    
    info = ProfilingInfo.objects.all()
    if len(info) > 0:
        profiling['updated'] = info[0].updated
        profiling['period']  = info[0].period
    else:
        profiling['updated'] = False
        profiling['period']  = False
    
    profiles = ProfilingSnapshot.objects.filter(cnt__gt = 0).order_by('total')
    profiling['profiles'] = profiles.all()
    
    return profiling
