import datetime
import time
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.db.models import Q, Count, Avg
from django.http import Http404
from django.shortcuts import render, redirect
from ftsmon import forms
from ftsweb.models import Job, File


def jobIndex(httpRequest, states = ['FAILED', 'FINISHEDDIRTY', 'FINISHED', 'CANCELED', 'ACTIVE', 'STAGING'],
             additionalTitle = None):
    
    # If jobId is in the request, redirect directly
    if httpRequest.method == 'GET' and 'jobId' in httpRequest.GET:
        return redirect(to = "jobs/%s" % httpRequest.GET['jobId'])

    # Initialize forms
    searchJobForm = forms.JobSearchForm(httpRequest.GET)
    filterForm    = forms.FilterForm(httpRequest.GET)
    
    # Time filter
    hours = 12
    if filterForm.is_valid() and filterForm['time_window'].value():
        hours = int(filterForm['time_window'].value())
    notBefore = datetime.datetime.utcnow() -  datetime.timedelta(hours = hours)
        
    if additionalTitle is None:
        additionalTitle = '(from the last %dh)' % (hours)
        
    # Initial query
    jobs = Job.objects.filter(Q(finish_time__gte = notBefore) | Q(finish_time = None))
    jobs = jobs.extra(select = {'nullFinished': 'coalesce(t_job.finish_time, CURRENT_TIMESTAMP)'})
    
    # Filter
    if filterForm.is_valid():
        if filterForm['state'].value():
            states = filterForm['state'].value()
        
        if filterForm['vo'].value():
            jobs = jobs.filter(vo_name = filterForm['vo'].value())
        
        if filterForm['source_se'].value():
            jobs = jobs.filter(file__source_se = filterForm['source_se'].value())\
                       .values('job_id').annotate(nSourceMatches = Count('file__file_id'))

        if filterForm['dest_se'].value():
            jobs = jobs.filter(file__dest_se = filterForm['dest_se'].value())\
                       .values('job_id').annotate(nDestMatches = Count('file__file_id'))
                        
    jobs = jobs.filter(job_state__in = states)
    
    # Push needed fields
    jobs = jobs.values('job_id', 'submit_host', 'submit_time', 'job_state', 'finish_time',
                       'vo_name', 'source_space_token', 'space_token',
                       'priority', 'user_dn', 'reason',
                       'job_metadata', 'nullFinished', 'source_se', 'dest_se')
    # Ordering
    jobs = jobs.order_by('-nullFinished', '-submit_time')

    # Paginate
    paginator = Paginator(jobs, 50)
    try:
        if 'page' in httpRequest.GET: 
            jobs = paginator.page(httpRequest.GET.get('page'))
        else:
            jobs = paginator.page(1)
    except PageNotAnInteger:
        jobs = paginator.page(1)
    except EmptyPage:
        jobs = paginator.page(paginator.num_pages)
        
    # Remember extra attributes
    extra_args = filterForm.args()
        
    # Render
    return render(httpRequest, 'jobindex.html',
                  {'filterForm': filterForm,
                   'jobs': jobs,
                   'paginator': paginator,
                   'extra_args': extra_args,
                   'additionalTitle': additionalTitle,
                   'request': httpRequest})
  
  

def queue(httpRequest):
  transfers = File.objects.filter(file_state__in = ['SUBMITTED', 'READY'])
  transfers = transfers.order_by('-job__submit_time', '-file_id')
    
  # Paginate
  paginator = Paginator(transfers, 50)
  try:
    if 'page' in httpRequest.GET:
      transfers = paginator.page(httpRequest.GET.get('page'))
    else:
      transfers = paginator.page(1)
  except PageNotAnInteger:
      transfers = paginator.page(1)
  except EmptyPage:
    transfers = paginator.page(paginator.num_pages)
  
  return render(httpRequest, 'queue.html',
                {'transfers': transfers,
                 'paginator': paginator,
                 'request': httpRequest})



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



def staging(httpRequest):
  transfers = File.objects.filter(file_state = 'STAGING')
  transfers = transfers.order_by('-job__submit_time', '-file_id')
    
  # Paginate
  paginator = Paginator(transfers, 50)
  try:
    if 'page' in httpRequest.GET:
      transfers = paginator.page(httpRequest.GET.get('page'))
    else:
      transfers = paginator.page(1)
  except PageNotAnInteger:
      transfers = paginator.page(1)
  except EmptyPage:
    transfers = paginator.page(paginator.num_pages)
  
  return render(httpRequest, 'staging.html',
                {'transfers': transfers,
                 'paginator': paginator,
                 'request': httpRequest})


