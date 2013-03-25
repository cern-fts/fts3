from datetime import datetime, timedelta
from django.db.models import Q, Count, Avg
from django.shortcuts import render, redirect
from ftsweb.models import Job, File, ConfigAudit



STATES        = ['SUBMITTED', 'READY', 'ACTIVE', 'FAILED', 'FINISHED', 'CANCELED', 'STAGING']
ACTIVE_STATES = ['SUBMITTED', 'READY', 'ACTIVE', 'STAGING']



def configurationAudit(httpRequest):
    configs = ConfigAudit.objects.order_by('-datetime')
    return render(httpRequest, 'configurationAudit.html',
                  {'configs': configs})



def _getCountPerState(states, age = None):
	count = {}
	
	query = File.objects.filter(file_state__in = states)
	if age:
		query = query.filter(job_finished__gt = datetime.now() - age)
	query = query.values('file_state').annotate(number = Count('file_state'))
	
	for row in query:
		count[row['file_state'].lower()] = row['number']
		
	for s in filter(lambda s: s not in count, map(lambda s: s.lower(), states)):
		count[s] = 0
		
	# Couple of aggregations
	count['queued'] = count['submitted'] + count['ready']
	count['total'] = count['finished'] + count['failed'] + count['canceled']
		
	return count



def _getTransferAndSubmissionPerHost():
    hostnames = []
        
    submissions = {}
    for j in Job.objects.values('submit_host').annotate(count = Count('submit_host')):
        submissions[j['submit_host']] = j['count']
        hostnames.append(j['submit_host'])
        
    transfers = {}
    for t in File.objects.values('transferHost').annotate(count = Count('transferHost')):
        # Submitted do not have a transfer host!
        if t['transferHost']:
            transfers[t['transferHost']] = t['count']
            if t['transferHost'] not in hostnames:
                hostnames.append(t['transferHost'])
        
    servers = []
    for h in hostnames:
        if h in submissions: s = submissions[h]
        else:                s = 0
        if h in transfers:   t = transfers[h]
        else:                t = 0   
        servers.append({'hostname': h, 'submissions': s, 'transfers': t})
        
    return servers



def _getStateCountPerVo():
    perVoDict = {}
    for voJob in File.objects.values('file_state', 'job__vo_name').annotate(count = Count('file_state')):
        vo = voJob['job__vo_name']
        if vo not in perVoDict:
            perVoDict[vo] = []
        perVoDict[vo].append((voJob['file_state'], voJob['count']))
        
    perVo = []
    for (vo, states) in perVoDict.iteritems():
        perVo.append({'vo': vo, 'states': states})
    return perVo



def _getAllPairs(source = None, dest = None):
    pairs = []
    
    query = File.objects.values('source_se', 'dest_se')
    if source:
        query = query.filter(source_se = source)
    if dest:
        query = query.filter(dest_se = dest)
    
    for pair in query.distinct():
        pairs.append((pair['source_se'], pair['dest_se']))
    return pairs



def _getAveragePerPair(pairs):
    avg = {}
    
    for (source, dest) in pairs:
        pairAvg = File.objects.exclude(file_state__in = ACTIVE_STATES)\
                              .filter(source_se = source,
                                      dest_se = dest)\
                              .aggregate(Avg('tx_duration'), Avg('throughput'))
        avg[(source, dest)] = {'avgDuration': pairAvg['tx_duration__avg'],
                               'avgThroughput': pairAvg['throughput__avg']}
    
    return avg



def _getFilesInStatePerPair(pairs, states):
    statesPerPair = {}
    
    for (source, dest) in pairs:
        statesPerPair[(source, dest)] = []
        
        statesInPair = File.objects.filter(file_state__in = states,
                                     source_se = source,
                                     dest_se = dest)\
                             .values('file_state')\
                             .annotate(count = Count('file_state'))

        for st in statesInPair:
            statesPerPair[(source, dest)].append((st['file_state'], st['count']))
        
    return statesPerPair



def _getStatsPerPair(source_se = None, dest_se = None):
    allPairs      = _getAllPairs(source_se, dest_se)
    avgsPerPair   = _getAveragePerPair(allPairs)
    activePerPair = _getFilesInStatePerPair(allPairs, ACTIVE_STATES) 
        
    pairs = []
    for pair in sorted(allPairs):
        p = {'source': pair[0], 'destination': pair[1]}
        
        if pair in activePerPair:
            p['active'] = activePerPair[pair]
            
        if pair in avgsPerPair:
            p.update(avgsPerPair[pair])
            
        pairs.append(p)
    return pairs



def statistics(httpRequest):
    statsDict = {}
    statsDict['request'] = httpRequest
    
    # Filters
    source_se = httpRequest.GET['source_se'] if 'source_se' in httpRequest.GET else None  
    dest_se   = httpRequest.GET['dest_se'] if 'dest_se' in httpRequest.GET else None    
    
    # Overall (all times)
    overall = _getCountPerState(STATES)        
    
    # Success rate (last hour)
    lastHour = _getCountPerState(STATES, timedelta(hours = 1))
    if lastHour['total'] > 0:
        overall['rate'] = (lastHour['finished'] * 100.0) / lastHour['total']
    else:
        overall['rate'] = 0
        
    statsDict['overall'] = overall
    
    statsDict['servers'] = _getTransferAndSubmissionPerHost()
    statsDict['pairs'] = _getStatsPerPair(source_se, dest_se)   
    statsDict['vos'] = _getStateCountPerVo();
    
    # Render
    return render(httpRequest, 'statistics.html',
                  statsDict)
 