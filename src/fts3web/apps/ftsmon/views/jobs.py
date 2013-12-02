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
from util import getOrderBy, orderedField
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
               'source_surl': None,
               'dest_surl': None,
               'metadata': None,
               'startdate': None,
               'enddate': None,
               'activity': None
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
    jobs = jobModel.objects
    
    # Convert startdate and enddate to datetime
    startdate = enddate = None
    if filters['startdate']:
        startdate = datetime.datetime.combine(filters['startdate'], datetime.time(0, 0, 0))
    if filters['enddate']:
        enddate = datetime.datetime.combine(filters['enddate'], datetime.time(23, 59, 59))
    
    # Time filter
    if filters['time_window']:
        notBefore = datetime.datetime.utcnow() -  datetime.timedelta(hours = filters['time_window'])
        jobs = jobs.filter(Q(job_finished__gte = notBefore) | Q(job_finished = None))
    elif startdate and enddate:
        jobs = jobs.filter(job_finished__gte = startdate, job_finished__lte = enddate)
    elif startdate:
        jobs = jobs.filter(job_finished__gte = startdate)
    elif enddate:
        jobs = jobs.filter(job_finished__lte = enddate)
    
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
    jobs = jobs.values('job_id', 'submit_host', 'submit_time', 'job_state', 'job_finished',
                       'vo_name', 'source_space_token', 'space_token',
                       'priority', 'user_dn', 'reason',
                       'job_metadata', 'source_se', 'dest_se')
    
    # Ordering
    (orderBy, orderDesc) = getOrderBy(httpRequest)

    if orderBy == 'submit_time' and not orderDesc:
        jobs = jobs.order_by('submit_time')
    else:    
        jobs = jobs.order_by('-submit_time')
    
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


@jsonify
def jobDetails(httpRequest, jobId):
    reason = httpRequest.GET.get('reason', None)
    try:
        job = Job.objects.get(job_id = jobId)
        count = File.objects.filter(job = jobId).values('file_state').annotate(count = Count('file_state'))
        if reason:
            count = count.filter(reason = reason)
    except Job.DoesNotExist:
        try:
            job = JobArchive.objects.get(job_id = jobId)
            count = FileArchive.objects.filter(job = jobId).values('file_state').annotate(count = Count('file_state'))
            if reason:
                count = count.filter(reason = reason)
        except JobArchive.DoesNotExist:
            raise Http404
    
    # Count as dictionary
    stateCount = {}
    for st in count:
        stateCount[st['file_state']] = st['count']
        
    return {'job': job, 'states': stateCount}


@jsonify_paged
def jobFiles(httpRequest, jobId):
    files = File.objects.filter(job = jobId)
    if not files:
        files = FileArchive.objects.filter(job = jobId)
    if not files:
        raise Http404
    
    if httpRequest.GET.get('state', None):
        files = files.filter(file_state__in = httpRequest.GET['state'].split(','))
        
    if httpRequest.GET.get('reason', None):
        files = files.filter(reason = httpRequest.GET['reason'])
        
    # Ordering
    (orderBy, orderDesc) = getOrderBy(httpRequest)
    if orderBy == 'id':
        files = files.order_by(orderedField('file_id', orderDesc))
    elif orderBy == 'size':
        files = files.order_by(orderedField('filesize', orderDesc))
    elif orderBy == 'throughput':
        files = files.order_by(orderedField('throughput', orderDesc))
    elif orderBy == 'start_time':
        files = files.order_by(orderedField('start_time', orderDesc))
    elif orderBy == 'end_time':
        files = files.order_by(orderedField('end_time', orderDesc))
        
    return files


@jsonify_paged
def transferList(httpRequest):
    filterForm = forms.FilterForm(httpRequest.GET)
    filters    = setupFilters(filterForm)
    
    # Force a time window
    if not filters['time_window']:
        filters['time_window'] = 12
    
    transfers = File.objects
    if filters['state']:
        transfers = transfers.filter(file_state__in = filters['state'])
    else:
        transfers = transfers.exclude(file_state = 'NOT_USED')
    if filters['source_se']:
        transfers = transfers.filter(source_se = filters['source_se'])
    if filters['dest_se']:
        transfers = transfers.filter(dest_se = filters['dest_se'])
    if filters['source_surl']:
        transfers = transfers.filter(source_surl = filters['source_surl'])
    if filters['dest_surl']:
        transfers = transfers.filter(dest_surl = filters['dest_surl'])
    if filters['vo']:
        transfers = transfers.filter(job__vo_name = filters['vo'])
    if filters['time_window']:
        notBefore =  datetime.datetime.utcnow() - datetime.timedelta(hours = filters['time_window'])
        transfers = transfers.filter(Q(job_finished__isnull = True) | (Q(job_finished__gte = notBefore)))
    if filters['activity']:
        transfers = transfers.filter(activity = filters['activity'])
   
    transfers = transfers.values('file_id', 'file_state', 'job_id',
                                 'source_se', 'dest_se', 'start_time', 'job_finished',
                                 'job__submit_time', 'job__priority')

    # Ordering
    (orderBy, orderDesc) = getOrderBy(httpRequest)
    if orderBy == 'id':
        transfers = transfers.order_by(orderedField('file_id', orderDesc))
    elif orderBy == 'priority':
        transfers = transfers.order_by(orderedField('job__priority', orderDesc))
    elif orderBy == 'submit_time':
        transfers = transfers.order_by(orderedField('job__submit_time', orderDesc))
    elif orderBy == 'start_time':
        transfers = transfers.order_by(orderedField('start_time', orderDesc))
    elif orderBy == 'finish_time':
        transfers = transfers.order_by(orderedField('job_finished', orderDesc))
    else:
        transfers = transfers.order_by('-job__priority', '-file_id')

    return transfers
