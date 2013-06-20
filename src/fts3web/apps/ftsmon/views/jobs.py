from django.core.paginator import Paginator
from django.db.models import Q, Count, Avg
from django.http import Http404
from django.shortcuts import render, redirect
from ftsmon import forms
from ftsweb.models import Job, File, JobArchive, FileArchive
from utils import getPage
import datetime
import json
import time



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
               'time_window': None,
               'vo': None,
               'source_se': None,
               'dest_se': None,
               'metadata': None,
               'startdate': None,
               'enddate': None
              }
    
    # Process filter form
    if filterForm.is_valid():
        for key in filters.keys():
            filters[key] = filterForm.cleaned_data[key]
                
    # If enddate < startdate, swap
    if filters['startdate'] and filters['enddate'] and filters['startdate'] > filters['enddate']:
        filters['startdate'], filters['enddate'] = filters['enddate'], filters['startdate']
    
    return filters



def jobListing(httpRequest, jobModel = Job, filters = None):
    # Initial query
    jobs = jobModel.objects.extra(select = {'nullFinished': 'coalesce(finish_time, CURRENT_TIMESTAMP)'})
    
    # Convert startdate and enddate to datetime
    startdate = enddate = None
    if filters['startdate']:
        startdate = datetime.datetime.combine(filters['startdate'], datetime.time(0, 0, 0))
    if filters['enddate']:
        enddate = datetime.datetime.combine(filters['enddate'], datetime.time(23, 59, 59))
    
    # Time filter
    if filters['time_window']:
        notBefore = datetime.datetime.utcnow() -  datetime.timedelta(hours = filters['time_window'])
        jobs = jobs.filter(Q(finish_time__gte = notBefore) | Q(finish_time = None))
    elif startdate and enddate:
        jobs = jobs.filter(finish_time__gte = startdate, finish_time__lte = enddate)
    elif startdate:
        jobs = jobs.filter(finish_time__gte = startdate)
    elif enddate:
        jobs = jobs.filter(finish_time__lte = enddate)
    
    # Filters
    if filters['vo']:
        jobs = jobs.filter(vo_name = filters['vo'])
        
    if filters['source_se']:
        jobs = jobs.filter(source_se = filters['source_se'])

    if filters['dest_se']:
        jobs = jobs.filter(dest_se = filters['dest_se'])

    if filters['state']:
        jobs = jobs.filter(job_state__in = filters['state'])
    
    # Push needed fields
    jobs = jobs.values('job_id', 'submit_host', 'submit_time', 'job_state', 'finish_time',
                       'vo_name', 'source_space_token', 'space_token',
                       'priority', 'user_dn', 'reason',
                       'job_metadata', 'nullFinished', 'source_se', 'dest_se')
    # Ordering
    jobs = jobs.order_by('-nullFinished', '-submit_time')
    
    # Wrap with a metadata filterer
    msg = None
    
    META_LIMIT = 2000
    if filters['metadata']:
        if jobs.count() < META_LIMIT:
            metadataFilter = MetadataFilter(filters['metadata'])
            jobs = filter(metadataFilter, jobs)
        else:
            msg = 'Can not filter by metadata with more than %d results' % META_LIMIT

    # Return list
    return (msg, jobs)



def jobIndex(httpRequest):
    states = ['FAILED', 'FINISHEDDIRTY', 'FINISHED', 'CANCELED', 'ACTIVE', 'STAGING']
    
    # If jobId is in the request, redirect directly
    if httpRequest.method == 'GET' and 'jobId' in httpRequest.GET:
        return redirect('ftsmon.views.jobDetails', jobId = httpRequest.GET['jobId'])
    
    filterForm = forms.FilterForm(httpRequest.GET)
    filters = setupFilters(filterForm)
    
    # Set some defaults for filters if they are empty
    if not filters['time_window']:
        filters['time_window'] = 12;
    if not filters['state']:
        filters['state'] = states
    
    msg, jobs = jobListing(httpRequest, filters = filters)    
    paginator = Paginator(jobs, 50)
    return render(httpRequest, 'jobs/list.html',
                  {'filterForm': filterForm,
                   'jobs':       getPage(paginator, httpRequest),
                   'paginator':  paginator,
                   'filters':    filters,
                   'message':    msg,
                   'request':    httpRequest})



def archiveJobIndex(httpRequest):
    filterForm = forms.FilterForm(httpRequest.GET)
    filters = setupFilters(filterForm)
    filters['time_window'] = None
    
    msg, jobs = jobListing(httpRequest, jobModel = JobArchive, filters = filters)    
    paginator = Paginator(jobs, 50)
    return render(httpRequest, 'jobs/archive.html',
                  {'filterForm': filterForm,
                   'jobs':       getPage(paginator, httpRequest),
                   'paginator':  paginator,
                   'filters':    filters,
                   'message':    msg,
                   'request':    httpRequest})



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
    
    return render(httpRequest, 'jobs/details.html',
                  {'transferJob': job,
                   'transferFiles': files,
                   'request': httpRequest})



def staging(httpRequest):
  transfers = File.objects.filter(file_state = 'STAGING')
  transfers = transfers.order_by('-job__submit_time', '-file_id')
  paginator = Paginator(transfers, 50)
  return render(httpRequest, 'jobs/staging.html',
                {'transfers': getPage(paginator, httpRequest),
                 'paginator': paginator,
                 'request': httpRequest})

