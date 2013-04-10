from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.db.models import Q, Count, Avg
from django.http import Http404
from django.shortcuts import render, redirect
from ftsmon import forms
from ftsweb.models import Job, File
import datetime
import json
import time


def _getPage(paginator, request):
    try:
        if 'page' in request.GET: 
            page = paginator.page(request.GET.get('page'))
        else:
            page = paginator.page(1)
    except PageNotAnInteger:
        page = paginator.page(1)
    except EmptyPage:
        page = paginator.page(paginator.num_pages)
    return page



class MetadataFilter:
    def __init__(self, filter):
        self.filter = filter
        
    def _compare(self, filter, meta):
        if isinstance(filter, dict) and isinstance(meta, dict):
            for (key, value) in filter.iteritems():
                if key not in meta or not self._compare(value, meta[key]):
                    return False
            return True
        elif filter == u'*':
            return True
        else:
            return filter == meta
        
    def __call__(self, item):
        try:
            value = item['job_metadata']
            if value:
                return self._compare(self.filter, json.loads(value))
        except:
            raise
        return False
    


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
    metadataFilter = None
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
                       
        if filterForm['metadata'].value():
            metadataFilter = MetadataFilter(filterForm['metadata'].value())
            
                        
    jobs = jobs.filter(job_state__in = states)
    
    # Push needed fields
    jobs = jobs.values('job_id', 'submit_host', 'submit_time', 'job_state', 'finish_time',
                       'vo_name', 'source_space_token', 'space_token',
                       'priority', 'user_dn', 'reason',
                       'job_metadata', 'nullFinished', 'source_se', 'dest_se')
    # Ordering
    jobs = jobs.order_by('-nullFinished', '-submit_time')[:1000]
    
    # Wrap with a metadata filterer
    if metadataFilter:
        jobs = filter(metadataFilter, jobs)

    # Paginate
    paginator = Paginator(jobs, 50)
        
    # Render
    return render(httpRequest, 'jobindex.html',
                  {'filterForm': filterForm,
                   'jobs':       _getPage(paginator, httpRequest),
                   'paginator':  paginator,
                   'additionalTitle': additionalTitle,
                   'request':    httpRequest})
  
  

def queue(httpRequest):
  transfers = File.objects.filter(file_state__in = ['SUBMITTED', 'READY'])
  transfers = transfers.order_by('-job__submit_time', '-file_id')
  paginator = Paginator(transfers, 50)
  return render(httpRequest, 'queue.html',
                {'transfers': _getPage(paginator, httpRequest),
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
  paginator = Paginator(transfers, 50)
  return render(httpRequest, 'staging.html',
                {'transfers': _getPage(paginator, httpRequest),
                 'paginator': paginator,
                 'request': httpRequest})

