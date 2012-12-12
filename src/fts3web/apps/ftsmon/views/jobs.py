import datetime
import time
from django.db.models import Q, Count, Avg
from django.http import Http404
from django.shortcuts import render, redirect
from ftsmon import forms
from fts3.models import Job, File, ConfigAudit


def jobIndex(httpRequest, states = ['FAILED', 'FAILEDDIRTY', 'FINISHED', 'CANCELED', 'ACTIVE'],
             additionalTitle = '(running or finished in the last 12h)'):
    
    notBefore = datetime.datetime.now() - datetime.timedelta(hours = 12)
    
    # If jobId is in the request, redirect directly
    if httpRequest.method == 'GET' and 'jobId' in httpRequest.GET:
        return redirect(to = "jobs/%s" % httpRequest.GET['jobId'])

    # Initialize forms
    searchJobForm = forms.JobSearchForm(httpRequest.GET)
    filterForm    = forms.FilterForm(httpRequest.GET)

    # Initial query
    jobs = Job.objects.filter(Q(job_state__in = states), Q(finish_time__gte = notBefore) | Q(finish_time = None))
    
    # Filter
    if filterForm.is_valid():
        if filterForm['state'].value():
            states = filterForm['state'].value()
        
        if filterForm['vo'].value():
            jobs = jobs.filter(vo_name = filterForm['vo'].value())
        
        if filterForm['source_se'].value():
            jobs = jobs.filter(source_se = filterForm['source_se'].value())
    
        if filterForm['dest_se'].value():
            jobs = jobs.filter(dest_se = filterForm['dest_se'].value())
    
    # Ordering
    jobs = jobs.order_by('-finish_time', '-submit_time')
    jobs = jobs.extra(select = {'nullFinished': 'coalesce(finish_time, CURRENT_TIMESTAMP)'}, order_by=['-nullFinished'])
        
    # Render
    return render(httpRequest, 'jobindex.html',
                  {'filterForm': filterForm,
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


