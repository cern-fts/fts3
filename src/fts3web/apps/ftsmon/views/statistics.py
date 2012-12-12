from django.db.models import Q, Count, Avg
from django.shortcuts import render, redirect
from fts3.models import Job, File, ConfigAudit



def configurationAudit(httpRequest):
    configs = ConfigAudit.objects.order_by('-datetime')
    return render(httpRequest, 'configurationAudit.html',
                  {'configs': configs})



def statistics(httpRequest):
    statsDict = {}
    statsDict['request'] = httpRequest
    
    STATES        = ['SUBMITTED', 'READY', 'ACTIVE', 'FAILED', 'FINISHED', 'CANCELED']
    ACTIVE_STATES = ['SUBMITTED', 'READY', 'ACTIVE']
    
    # Overall
    overall = {}
    for s in STATES:
        label = s.lower()
        overall[label] = File.objects.filter(file_state = s).count()
        
    overall['queued'] = overall['submitted'] + overall['ready']
    total = overall['finished'] + overall['failed'] + overall['canceled']
    if total > 0:
        overall['rate'] = (overall['finished'] * 100.0) / total
    else:
        overall['rate'] = 0
        
    statsDict['overall'] = overall
    
    # Unique error reasons
    statsDict['uniqueReasons'] = File.objects.filter(reason__isnull = False).exclude(reason = '').\
                                   values('reason').annotate(count = Count('reason')).order_by('-count')
                                   
    # Submissions and transfers handled per each FTS3 machine
    hostnames = []
        
    submissions = {}
    for j in Job.objects.values('submitHost').annotate(count = Count('submitHost')):
        submissions[j['submitHost']] = j['count']
        hostnames.append(j['submitHost'])
        
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
    sePairs = []
    
    avgsPerPair = {}
    for pair in Job.objects.exclude(job_state__in = ACTIVE_STATES).values('source_se', 'dest_se').distinct():
        pair_tuple = (pair['source_se'], pair['dest_se'])
        sePairs.append(pair_tuple)
        
        pairAvg = File.objects.exclude(file_state__in = ACTIVE_STATES).filter(
                                      job__source_se = pair_tuple[0],
                                      job__dest_se = pair_tuple[1]).aggregate(Avg('tx_duration'), Avg('throughput'))
        avgsPerPair[pair_tuple] = {'avgDuration': pairAvg['tx_duration__avg'], 'avgThroughput': pairAvg['throughput__avg']}
    
    activePerPair = {}
    for pair in Job.objects.filter(job_state__in = ACTIVE_STATES).values('source_se', 'dest_se', 'job_state').annotate(count = Count('job_state')):
        pair_tuple = (pair['source_se'], pair['dest_se'])
        state      = pair['job_state']
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
    for voJob in Job.objects.values('vo_name', 'job_state').annotate(count = Count('job_state')):
        vo = voJob['vo_name']
        if vo not in perVoDict:
            perVoDict[vo] = []
        perVoDict[vo].append((voJob['job_state'], voJob['count']))
        
    perVo = []
    for (vo, states) in perVoDict.iteritems():
        perVo.append({'vo': vo, 'states': states})
    
    statsDict['vos'] = perVo
    
    # Render
    return render(httpRequest, 'statistics.html',
                  statsDict)
 