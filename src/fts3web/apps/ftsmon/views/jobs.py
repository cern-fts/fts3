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
from ftsweb.models import Job, File, JobArchive, FileArchive, RetryError
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
    


def setupFilters(httpRequest):
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
               'activity': None,
               'hostname': None,
               'reason': None
              }
    
    for key in filters.keys():
        try:
            value = httpRequest.GET.get(key, None)
            if value:
                if key == 'time_window':
                    filters[key] = int(httpRequest.GET[key])
                elif key == 'state':
                    filters[key] = httpRequest.GET[key].split(',')
                else:
                    filters[key] = httpRequest.GET[key]
        except:
            pass
                
    # If enddate < startdate, swap
    if filters['startdate'] and filters['enddate'] and filters['startdate'] > filters['enddate']:
        filters['startdate'], filters['enddate'] = filters['enddate'], filters['startdate']
    
    return filters



def jobListing(httpRequest, jobModel = Job):
    filters = setupFilters(httpRequest)
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
    msg, jobs = jobListing(httpRequest)
    return jobs


@jsonify_paged
def archiveJobIndex(httpRequest):
    msg, jobs = jobListing(httpRequest, jobModel = JobArchive, filters = filters)
    return jobs


@jsonify
def jobDetails(httpRequest, jobId):
    reason  = httpRequest.GET.get('reason', None)
    try:
        file_id = httpRequest.GET.get('file', None)
        if file_id is not None:
            file_id = int(file_id)
    except:
        file_id = None
        
    try:
        archived = int(httpRequest.GET.get('archive', 0))
    except:
        archived = 0

    try:
        job = Job.objects.get(job_id = jobId)
        count = File.objects.filter(job = jobId)
    except Job.DoesNotExist:
        if archived:
            try:
                job = JobArchive.objects.get(job_id = jobId)
                count = FileArchive.objects.filter(job = jobId)
            except JobArchive.DoesNotExist:
                raise Http404
        else:
            raise Http404
        
    if reason:
        count = count.filter(reason = reason)
    if file_id:
        count = count.filter(file_id = file_id)
    count = count.values('file_state').annotate(count = Count('file_state'))
    
    # Count as dictionary
    stateCount = {}
    for st in count:
        stateCount[st['file_state']] = st['count']
        
    return {'job': job, 'states': stateCount}


class RetriesFetcher(object):
    """
    Fetches, on demand and if necessary, the retry error messages
    """
    
    def __init__(self, files):
        self.files = files
        
    def __len__(self):
        return len(self.files)
    
    def __getitem__(self, i):
        for f in self.files[i]:
            retries = RetryError.objects.filter(file_id = f.file_id)
            f.retries = map(lambda r: {
                   'reason': r.reason,
                   'datetime': r.datetime,
                   'attempt': r.attempt
            }, retries.all())
            yield f


@jsonify_paged
def jobFiles(httpRequest, jobId):
    files = File.objects.filter(job = jobId)
    try:
        archived = int(httpRequest.GET.get('archive', 0))
    except:
        archived = 0

    if not files and archived:
        files = FileArchive.objects.filter(job = jobId)
    if not files:
        raise Http404
    
    if httpRequest.GET.get('state', None):
        files = files.filter(file_state__in = httpRequest.GET['state'].split(','))
    if httpRequest.GET.get('reason', None):
        files = files.filter(reason = httpRequest.GET['reason'])
    if httpRequest.GET.get('file', None):
        files = files.filter(file_id = httpRequest.GET['file'])
        
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
        
    return RetriesFetcher(files)
