from datetime import datetime, timedelta
from django.db.models import Q, Count, Avg
from django.shortcuts import render, redirect
from fts3.models import Job, File, ConfigAudit



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
		
	for s in filter(lambda s: s.lower() not in count, states):
		count[s.lower()] = 0
		
	# Couple of aggregations
	count['queued'] = count['submitted'] + count['ready']
	count['total'] = count['finished'] + count['failed'] + count['canceled']
		
	return count


def statistics(httpRequest):
    statsDict = {}
    statsDict['request'] = httpRequest
    
    STATES        = ['SUBMITTED', 'READY', 'ACTIVE', 'FAILED', 'FINISHED', 'CANCELED', 'STAGING']
    ACTIVE_STATES = ['SUBMITTED', 'READY', 'ACTIVE', 'STAGING']
    
    # Overall (all times)
    overall = _getCountPerState(STATES)        
    
    # Success rate (last hour)
    lastHour = _getCountPerState(STATES, timedelta(hours = 1))
    if lastHour['total'] > 0:
        overall['rate'] = (lastHour['finished'] * 100.0) / lastHour['total']
    else:
        overall['rate'] = 0
        
    statsDict['overall'] = overall
    
    # Unique error reasons
    statsDict['uniqueReasons'] = File.objects.filter(reason__isnull = False).exclude(reason = '').\
                                   values('reason').annotate(count = Count('reason')).order_by('-count')
                                   
    # Submissions and transfers handled per each FTS3 machine
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
        
    statsDict['servers'] = servers
    
    # Stats per pair of SEs
    pairQuery = Job.objects.exclude(job_state__in = ACTIVE_STATES).values('source_se', 'dest_se')
    
    if 'source_se' in httpRequest.GET:
        pairQuery = pairQuery.filter(source_se = httpRequest.GET['source_se'])
    if 'dest_se' in httpRequest.GET:
        pairQuery = pairQuery.filter(dest_se = httpRequest.GET['dest_se'])
    
    pairQuery = pairQuery.distinct()
    
    sePairs = []    
    avgsPerPair = {}
    for pair in pairQuery:
        pair_tuple = (pair['source_se'], pair['dest_se'])
        sePairs.append(pair_tuple)
        
        pairAvg = File.objects.exclude(file_state__in = ACTIVE_STATES).filter(
                                      job__source_se = pair_tuple[0],
                                      job__dest_se = pair_tuple[1]).aggregate(Avg('tx_duration'), Avg('throughput'))
        avgsPerPair[pair_tuple] = {'avgDuration': pairAvg['tx_duration__avg'], 'avgThroughput': pairAvg['throughput__avg']}
    
    
    activeQuery = File.objects.filter(file_state__in = ACTIVE_STATES).values('job__source_se', 'job__dest_se', 'file_state')

    if 'source_se' in httpRequest.GET:
        activeQuery = activeQuery.filter(job__source_se = httpRequest.GET['source_se'])
    if 'dest_se' in httpRequest.GET:
        activeQuery = activeQuery.filter(job__dest_se = httpRequest.GET['dest_se'])
    
    activeQuery = activeQuery.annotate(count = Count('file_state'))
    
    activePerPair = {}
    for pair in activeQuery:
        pair_tuple = (pair['job__source_se'], pair['job__dest_se'])
        state      = pair['file_state']
        count      = pair['count']
        
        if pair_tuple not in sePairs:
            sePairs.append(pair_tuple)
        
        if pair_tuple not in activePerPair:
            activePerPair[pair_tuple] = []
            
        activePerPair[pair_tuple].append((state, count))
        
    pairs = []
    for pair in sorted(sePairs):
        p = {'source': pair[0], 'destination': pair[1]}
        
        if pair in activePerPair:
            p['active'] = activePerPair[pair]
            
        if pair in avgsPerPair:
            p.update(avgsPerPair[pair])
            
        pairs.append(p)
    
    statsDict['pairs'] = pairs
    
    # State per VO    
    perVoDict = {}
    for voJob in File.objects.values('file_state', 'job__vo_name').annotate(count = Count('file_state')):
        vo = voJob['job__vo_name']
        if vo not in perVoDict:
            perVoDict[vo] = []
        perVoDict[vo].append((voJob['file_state'], voJob['count']))
        
    perVo = []
    for (vo, states) in perVoDict.iteritems():
        perVo.append({'vo': vo, 'states': states})
    
    statsDict['vos'] = perVo
    
    # Render
    return render(httpRequest, 'statistics.html',
                  statsDict)
 