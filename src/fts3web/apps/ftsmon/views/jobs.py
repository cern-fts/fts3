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
from django.db import connection
from django.db.models import Q, Count, Avg
from django.http import Http404
from django.shortcuts import render, redirect
from jsonify import jsonify, jsonify_paged
from ftsweb.models import Job, File, RetryError
from util import getOrderBy, orderedField
import datetime
import json
import time
    


def setupFilters(httpRequest):
    # Default values
    filters = {'state': None,
               'time_window': 1,
               'vo': None,
               'source_se': None,
               'dest_se': None,
               'source_surl': None,
               'dest_surl': None,
               'metadata': None,
               'activity': None,
               'hostname': None,
               'reason': None,
               'with_file': None
              }
    
    for key in filters.keys():
        try:
            value = httpRequest.GET.get(key, None)
            if value:
                if key == 'time_window':
                    filters[key] = int(httpRequest.GET[key])
                elif key == 'state' or key == 'with_file':
                    filters[key] = httpRequest.GET[key].split(',')
                else:
                    filters[key] = httpRequest.GET[key]
        except:
            pass
    
    return filters


class JobListDecorator(object):
    """
    Wraps the list of jobs and appends additional information, as
    file count per state
    This way we only do it for the number that is being actually sent
    """
    def __init__(self, job_ids):
        self.job_ids = job_ids
    
    def __len__(self):
        return len(self.job_ids)
    
    def _decorated(self, index):
        cursor = connection.cursor()
        for job_id in self.job_ids[index]:
            job = {'job_id': job_id}

            cursor.execute("SELECT submit_time, job_state, vo_name, source_se, dest_se, priority, space_token FROM t_job WHERE job_id = %s", [job_id])
            job_desc = cursor.fetchall()[0]
            job['submit_time'] = job_desc[0]
            job['job_state'] = job_desc[1]
            job['vo_name'] = job_desc[2]
            job['source_se'] = job_desc[3]
            job['dest_se'] = job_desc[4]
            job['priority'] = job_desc[5]
            job['space_token'] = job_desc[6]

            cursor.execute("SELECT file_state, COUNT(file_state) FROM t_file WHERE job_id = %s GROUP BY file_state ORDER BY NULL", [job_id])
            result = cursor.fetchall()
            count = dict()
            for r in result:
                count[r[0]] = r[1]
            job['files'] = count
            yield job
    
    def __getitem__(self, index):
        if not isinstance(index, slice):
            index = slice(index, index, 1)
        
        step = index.step if index.step else 1
        nelems = (index.stop - index.start) / step
        if nelems > 100:
            return self.job_ids[index]
        else:
            return self._decorated(index)

@jsonify_paged
def jobIndex(httpRequest):
    """
    This view is a little bit trickier than the others.
    We get the list of job ids from t_job or t_file depending if
    filtering by state of with_file is used.
    Luckily, the rest of fields are available in both tables
    """
    filters = setupFilters(httpRequest)

    if filters['with_file']:
        job_ids = File.objects.values('job_id').distinct().filter(file_state__in = filters['with_file'])
    elif filters['state']:
        job_ids = Job.objects.values('job_id').filter(job_state__in = filters['state']).order_by('-submit_time')
    else:
        job_ids = Job.objects.values('job_id').order_by('-submit_time')
        
    if filters['time_window']:
        notBefore = datetime.datetime.utcnow() -  datetime.timedelta(hours = filters['time_window'])
        job_ids = job_ids.filter(Q(job_finished__gte = notBefore) | Q(job_finished = None))

    if filters['vo']:
        job_ids = job_ids.filter(vo_name = filters['vo'])
    if filters['source_se']:
        job_ids = job_ids.filter(source_se = filters['source_se'])
    if filters['dest_se']:
        job_ids = job_ids.filter(dest_se = filters['dest_se'])

    # Prefetch to avoid count(*) query
    return JobListDecorator(map(lambda j: j['job_id'], job_ids))


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
        job = Job.objects.get(job_id = jobId)
        count = File.objects.filter(job = jobId)
    except Job.DoesNotExist:
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


@jsonify_paged
def transferList(httpRequest):
    filters    = setupFilters(httpRequest)
    
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
        transfers = transfers.filter(vo_name = filters['vo'])
    if filters['time_window']:
        notBefore =  datetime.datetime.utcnow() - datetime.timedelta(hours = filters['time_window'])
        transfers = transfers.filter(Q(job_finished__isnull = True) | (Q(job_finished__gte = notBefore)))
    if filters['activity']:
        transfers = transfers.filter(activity = filters['activity'])
    if filters['hostname']:
        transfers = transfers.filter(transferHost = filters['hostname'])
    if filters['reason']:
        transfers = transfers.filter(reason = filters['reason'])
   
    transfers = transfers.values('file_id', 'file_state', 'job_id',
                                 'source_se', 'dest_se', 'start_time', 'finish_time')

    # Ordering
    (orderBy, orderDesc) = getOrderBy(httpRequest)
    if orderBy == 'id':
        transfers = transfers.order_by(orderedField('file_id', orderDesc))
    elif orderBy == 'start_time':
        transfers = transfers.order_by(orderedField('start_time', orderDesc))
    elif orderBy == 'finish_time':
        transfers = transfers.order_by(orderedField('finish_time', orderDesc))
    else:
        transfers = transfers.order_by('-file_id')

    return transfers
