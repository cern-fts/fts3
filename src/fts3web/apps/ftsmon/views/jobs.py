from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.db.models import Q, Count, Avg
from django.http import Http404
from django.shortcuts import render, redirect
from ftsmon import forms
from ftsweb.models import Job, File, JobArchive, FileArchive
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
    


def setupFilters(filterForm):
    # Default values
    filters = {'state': None,
               'hours': None,
               'vo': None,
               'source_se': None,
               'dest_se': None,
               'metadata': None
              }
    
    # Process filter form
    if filterForm.is_valid():
        if filterForm['time_window'].value():
            filters['hours'] = int(filterForm['time_window'].value())
            
        for key in ('state', 'vo', 'source_se', 'dest_se', 'metadata'):
            if filterForm[key].value():
                filters[key] = filterForm[key].value()
    
    return filters



def jobListing(httpRequest, jobModel = Job, filters = None):
    # Initial query
    jobs = jobModel.objects.extra(select = {'nullFinished': 'coalesce(finish_time, CURRENT_TIMESTAMP)'})
    
    # Time filter
    if filters['hours']:
        notBefore = datetime.datetime.utcnow() -  datetime.timedelta(hours = filters['hours'])
        jobs = jobs.filter(Q(finish_time__gte = notBefore) | Q(finish_time = None))
    
    # Filters
    if filters['vo']:
        jobs = jobs.filter(vo_name = filters['vo'])
        
    if filters['source_se']:
        jobs = jobs.filter(file__source_se = filters['source_se'])\
                   .values('job_id').annotate(nSourceMatches = Count('file__file_id'))

    if filters['dest_se']:
        jobs = jobs.filter(file__dest_se = filters['dest_se'])\
                   .values('job_id').annotate(nDestMatches = Count('file__file_id'))

    if filters['state']:
        jobs = jobs.filter(job_state__in = filters['state'])
    
    # Push needed fields
    jobs = jobs.values('job_id', 'submit_host', 'submit_time', 'job_state', 'finish_time',
                       'vo_name', 'source_space_token', 'space_token',
                       'priority', 'user_dn', 'reason',
                       'job_metadata', 'nullFinished', 'source_se', 'dest_se')
    # Ordering
    jobs = jobs.order_by('-nullFinished', '-submit_time')[:1000]
    
    # Wrap with a metadata filterer
    if filters['metadata']:
        metadataFilter = MetadataFilter(filters['metadata'])
        jobs = filter(metadataFilter, jobs)

    # Return list
    return jobs



def jobIndex(httpRequest):
    states = ['FAILED', 'FINISHEDDIRTY', 'FINISHED', 'CANCELED', 'ACTIVE', 'STAGING']
    
    # If jobId is in the request, redirect directly
    if httpRequest.method == 'GET' and 'jobId' in httpRequest.GET:
        return redirect('ftsmon.views.jobDetails', jobId = httpRequest.GET['jobId'])
    
    filterForm = forms.FilterForm(httpRequest.GET)
    filters = setupFilters(filterForm)
    
    # Set some defaults for filters if they are empty
    if not filters['hours']:
        filters['hours'] = 12;
    if not filters['state']:
        filters['state'] = states
    
    jobs = jobListing(httpRequest, filters = filters)    
    paginator = Paginator(jobs, 50)
    return render(httpRequest, 'jobindex.html',
                  {'filterForm': filterForm,
                   'jobs':       _getPage(paginator, httpRequest),
                   'paginator':  paginator,
                   'hours': filters['hours'],
                   'request':    httpRequest})



def archiveJobIndex(httpRequest):
    filterForm = forms.FilterForm(httpRequest.GET)
    filters = setupFilters(filterForm)
    filters['hours'] = None
    
    jobs = jobListing(httpRequest, jobModel = JobArchive, filters = filters)    
    paginator = Paginator(jobs, 50)
    return render(httpRequest, 'jobarchive.html',
                  {'filterForm': filterForm,
                   'jobs':       _getPage(paginator, httpRequest),
                   'paginator':  paginator,
                   'additionalTitle': '',
                   'request':    httpRequest})



def queue(httpRequest):
  transfers = File.objects.filter(file_state__in = ['SUBMITTED', 'READY'])
  transfers = transfers.order_by('-job__submit_time', '-file_id')
  paginator = Paginator(transfers, 50)
  return render(httpRequest, 'queue.html',
                {'transfers': _getPage(paginator, httpRequest),
                 'paginator': paginator,
                 'request': httpRequest})



def _getJob(jobModel, fileModel, jobId):
    try:
        job   = jobModel.objects.get(job_id = jobId)
        files = fileModel.objects.filter(job = jobId)
        return (job, files)
    except jobModel.DoesNotExist:
        return (None, None)



def jobDetails(httpRequest, jobId):
    # Try t_job and t_file first
    (job, files) = _getJob(Job, File, jobId)
    # Otherwise, try the archive
    if not job:
        (job, files) = _getJob(JobArchive, FileArchive, jobId)
        
    if not job:
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

