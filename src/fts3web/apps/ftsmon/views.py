import datetime
import time
from django.db.models import Q, Count, Avg
from django.http import Http404
from django.shortcuts import render, redirect
from ftsmon import forms
from fts3.models import Job, File


def jobIndex(httpRequest, states = ['FAILED', 'FAILEDDIRTY', 'FINISHED', 'CANCELED', 'ACTIVE'],
             additionalTitle = '(running or finished in the last 12h)'):
    if httpRequest.method == 'GET' and 'jobId' in httpRequest.GET:
        form = forms.JobSearchForm(httpRequest.GET)
        if form.is_valid():
            return redirect(to = "jobs/%s" % form.cleaned_data['jobId'])
    else:
        form = forms.JobSearchForm()


    notBefore = datetime.datetime.now() - datetime.timedelta(hours = 12)
    if 'vo' in httpRequest.GET and str(httpRequest.GET['vo']) != '':
        jobs = Job.objects.filter(Q(job_state__in = states),
                                  Q(vo_name = str(httpRequest.GET['vo'])),
                                  Q(finish_time__gte = notBefore) | Q(finish_time = None))
    else:
        jobs = Job.objects.filter(Q(job_state__in = states),
                                  Q(finish_time__gte = notBefore) | Q(finish_time = None))
    
    jobs = jobs.order_by('-finish_time', '-submit_time')
    jobs = jobs.extra(select = {'nullFinished': 'coalesce(finish_time, CURRENT_TIMESTAMP)'}, order_by=['-nullFinished'])
        
    return render(httpRequest, 'jobindex.html',
                  {'form': form,
                   'jobs': jobs,
                   'additionalTitle': additionalTitle,
                   'request': httpRequest})
  
  

def jobQueue(httpRequest):
  return jobIndex(httpRequest, ['READY', 'SUBMITTED'], '')



def jobDetails(httpRequest, jobId):
    try:
        job = Job.objects.get(job_id = jobId)
        files = File.objects.filter(job = jobId)
    except Job.DoesNotExist:
        raise Http404
    
    return render(httpRequest, 'jobdetails.html',
                  {'transferJob': job,
                   'transferFiles': files,
                   'request': httpRequest})



def statistics(httpRequest):    
    statsDict = {}
    statsDict['request'] = httpRequest
    
    statsDict['numberOfFinished'] = File.objects.filter(file_state = 'FINISHED').count()
    statsDict['numberOfFailed']   = File.objects.filter(file_state__in = ['FAILED', 'CANCELED']).count()
    total = statsDict['numberOfFinished'] + statsDict['numberOfFailed']  
    if total != 0:
        statsDict['successRate'] = (statsDict['numberOfFinished'] / float(total)) * 100
    else:
        statsDict['successRate'] = "N/A"
    
    statsDict['numberOfActive'] = File.objects.filter(file_state = 'ACTIVE').count()    
    statsDict['numberOfQueued'] = File.objects.filter(file_state__in = ['READY', 'SUBMITTED']).count()
    
    statsDict['uniqueReasons'] = File.objects.filter(reason__isnull = False).exclude(reason = '').values('reason').annotate(count = Count('reason')).order_by('-count')
    
    perVo = {}
    for vo in Job.objects.values('vo_name').distinct('vo_name'):
        perVo[vo['vo_name']] = File.objects.filter(file_state = 'ACTIVE', job__vo_name = vo['vo_name']).count()        
    statsDict['activePerVO'] = perVo
    
    avgs = []
    for pair in Job.objects.filter(job_state = 'FINISHED').values('source_se', 'dest_se').distinct():
        source_se = pair['source_se']
        dest_se   = pair['dest_se']
        pairAvg = File.objects.filter(file_state = 'FINISHED',
                                      job__source_se = source_se,
                                      job__dest_se = dest_se).aggregate(Avg('tx_duration'), Avg('throughput'))
        avgs.append((source_se, dest_se, pairAvg['tx_duration__avg'], pairAvg['throughput__avg']))
        
    statsDict['avgPerPair'] = avgs
    
    statePerPair = {}
    activeStatus = ['SUBMITTED', 'READY', 'ACTIVE']
    for pair in Job.objects.filter(job_state__in = activeStatus).values('source_se', 'dest_se', 'job_state').annotate(count = Count('job_state')):
        pair_tuple = (pair['source_se'], pair['dest_se'])
        state     = pair['job_state']
        count     = pair['count']
        
        if pair_tuple not in statePerPair:
            statePerPair[pair_tuple] = []
            
        statePerPair[pair_tuple].append((state, count))
    
    statsDict['statePerPair'] = statePerPair

    return render(httpRequest, 'statistics.html',
                  statsDict)

