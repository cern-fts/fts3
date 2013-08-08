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
from django.shortcuts import render, redirect
from ftsweb.models import Job, File, ConfigAudit
from ftsweb.models import ProfilingSnapshot, ProfilingInfo


STATES               = ['SUBMITTED', 'READY', 'ACTIVE', 'FAILED', 'FINISHED', 'CANCELED', 'STAGING']
ACTIVE_STATES        = ['SUBMITTED', 'READY', 'ACTIVE', 'STAGING']
FILE_TERMINAL_STATES = ['FINISHED', 'FAILED', 'CANCELED']



def configurationAudit(httpRequest):
    configs = ConfigAudit.objects.order_by('-datetime')
    return render(httpRequest, 'configurationAudit.html',
                  {'configs': configs})



def _getCountPerState(states, age = None):
    count = {}
    
    query = File.objects.filter(file_state__in = states)
    if age:
        notBefore = datetime.utcnow() - age
        query = query.filter(Q(finish_time__gte = notBefore) | Q(finish_time__isnull = True))
    
    query = query.values('file_state').annotate(number = Count('file_state'))
    
    for row in query:
        count[row['file_state'].lower()] = row['number']
    
    for s in filter(lambda s: s not in count, map(lambda s: s.lower(), states)):
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
        
    servers = []
    for h in hostnames:
        if h in submissions: s = submissions[h]
        else:                s = 0
        if h in transfers:   t = transfers[h]
        else:                t = 0
        if h in active:      a = active[h]
        else:                a = 0
        servers.append({'hostname': h, 'submissions': s, 'transfers': t, 'active': a})
        
    return servers



def _getStateCountPerVo(timewindow):
    perVoDict = {}
    query = File.objects.values('file_state', 'job__vo_name')\
                        .filter(Q(finish_time__gte = datetime.utcnow() - timewindow) | Q(finish_time__isnull = True))\
                        .annotate(count = Count('file_state'))\
                        .order_by('file_state')
    for voJob in query:
        vo = voJob['job__vo_name']
        if vo not in perVoDict:
            perVoDict[vo] = []
        perVoDict[vo].append((voJob['file_state'], voJob['count']))
        
    perVo = []
    for (vo, states) in perVoDict.iteritems():
        perVo.append({'vo': vo, 'states': states})
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
    
    for (source, dest) in pairs:
        pairAvg = File.objects.exclude(file_state__in = ACTIVE_STATES, finish_time__gt = notBefore)\
                              .filter(source_se = source,
                                      dest_se = dest)\
                              .aggregate(Avg('tx_duration'), Avg('throughput'))
        avg[(source, dest)] = {'avgDuration': pairAvg['tx_duration__avg'],
                               'avgThroughput': pairAvg['throughput__avg']}
    
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
    
    statesPerPair = _getFilesInStatePerPair(allPairs, None, notBefore)
        
    pairs = []
    for pair in sorted(allPairs):
        p = {'source': pair[0], 'destination': pair[1]}

        if pair in avgsPerPair:
            p.update(avgsPerPair[pair])
            
        if pair in statesPerPair:
            states = statesPerPair[pair]
            p['active'] = {}
            # Active
            for k in ACTIVE_STATES:
                if k in states:
                    p['active'][k] = states[k]
                
            # Success rate
            terminal = dict((k, v) for k, v in states.items() if k in FILE_TERMINAL_STATES)
            total = float(reduce(lambda a,b: a+b, terminal.values(), 0))
            success = float(states['FINISHED'] if 'FINISHED' in states else 0)
            
            if total:
                p['successRate'] = (success/total) * 100
            else:
                p['successRate'] = None

        pairs.append(p)
    return pairs



def overview(httpRequest):
    overall = _getCountPerState(STATES, timedelta(hours = 24))
    lastHour = _getCountPerState(STATES, timedelta(hours = 1))
    if lastHour['total'] > 0:
        overall['rate'] = (lastHour['finished'] * 100.0) / lastHour['total']
    else:
        overall['rate'] = 0
        
    return render(httpRequest, 'statistics/overview.html',
                  {'overall': overall})
    


def servers(httpRequest):
    servers = _getTransferAndSubmissionPerHost(timedelta(hours = 12))
    return render(httpRequest, 'statistics/servers.html',
                  {'servers': servers})


def pairs(httpRequest):
    source_se = httpRequest.GET['source_se'] if 'source_se' in httpRequest.GET else None  
    dest_se   = httpRequest.GET['dest_se'] if 'dest_se' in httpRequest.GET else None    
    pairs = _getStatsPerPair(source_se, dest_se, timedelta(minutes = 30))
    return render(httpRequest, 'statistics/pairs.html',
                  {'pairs': pairs,
                   'request': httpRequest})



def pervo(httpRequest):
    vos = _getStateCountPerVo(timedelta(minutes = 30));
    return render(httpRequest, 'statistics/vos.html',
                  {'vos': vos})



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
    
    return render(httpRequest, 'statistics/profiling.html',
                  {'profiling': profiling,
                   'request': httpRequest})
