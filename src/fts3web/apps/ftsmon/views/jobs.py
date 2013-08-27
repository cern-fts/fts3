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

from django.db.models import Q, Count, Avg
from django.http import Http404
from django.shortcuts import render, redirect
from jsonify import jsonify, jsonify_paged
from ftsmon import forms
from ftsweb.models import Job, File, JobArchive, FileArchive
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
                try:
                    return self._compare(self.filter, json.loads(value))
                except:
                    return self._compare(self.filter, value)
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


@jsonify_paged
def jobIndex(httpRequest):
    states = ['FAILED', 'FINISHEDDIRTY', 'FINISHED', 'CANCELED', 'ACTIVE', 'STAGING']
    
    filterForm = forms.FilterForm(httpRequest.GET)
    filters = setupFilters(filterForm)
    
    # Set some defaults for filters if they are empty
    if not filters['time_window']:
        filters['time_window'] = 12;
    if not filters['state']:
        filters['state'] = states
    
    msg, jobs = jobListing(httpRequest, filters = filters)

    return jobs


@jsonify_paged
def archiveJobIndex(httpRequest):
    filterForm = forms.FilterForm(httpRequest.GET)
    filters = setupFilters(filterForm)
    filters['time_window'] = None
    
    msg, jobs = jobListing(httpRequest, jobModel = JobArchive, filters = filters)

    return jobs



def _getJob(jobModel, fileModel, jobId, fstate = None):
    try:
        job   = jobModel.objects.get(job_id = jobId)
        files = fileModel.objects.filter(job = jobId)
        if fstate:
            files = files.filter(file_state = fstate)
        return (job, files)
    except jobModel.DoesNotExist:
        return (None, None)


@jsonify
def jobDetails(httpRequest, jobId):
    # State filter
    state = None
    if 'state' in httpRequest.GET:
        state = httpRequest.GET['state']
    # Try t_job and t_file first
    (job, files) = _getJob(Job, File, jobId, state)
    # Otherwise, try the archive
    if not job:
        (job, files) = _getJob(JobArchive, FileArchive, jobId, state)
        
    if not job:
        raise Http404
    
    #transferStateCount = File.objects.filter(job = jobId)\
#                                     .values('file_state')\
                                     #.annotate(count = Count('file_state'))

    job.files = files                                   
    return job


@jsonify_paged
def staging(httpRequest):
    transfers = File.objects.filter(file_state = 'STAGING')
    transfers = transfers.order_by('-job__submit_time', '-file_id')
    transfers = transfers.extra(select = {'vo_name': 'vo_name', 'bring_online': 'bring_online', 'copy_pin_lifetime': 'copy_pin_lifetime'})
        
    return transfers

