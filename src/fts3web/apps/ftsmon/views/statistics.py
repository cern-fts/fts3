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

from datetime import datetime, timedelta
from django.db import connection
from django.db.models import Q, Count, Avg
from django.db.utils import DatabaseError
from ftsweb.models import Job, File, Host
from ftsweb.models import ProfilingSnapshot, ProfilingInfo
from jsonify import jsonify, jsonify_paged
from util import getOrderBy, orderedField
import settings


STATES               = ['SUBMITTED', 'READY', 'ACTIVE', 'FAILED', 'FINISHED', 'CANCELED', 'STAGING', 'NOT_USED']
ACTIVE_STATES        = ['SUBMITTED', 'READY', 'ACTIVE', 'STAGING']
FILE_TERMINAL_STATES = ['FINISHED', 'FAILED', 'CANCELED']


def _getCountPerState(age = None, hostname = None):
    count = {}
    
    query = File.objects
    if age:
        notBefore = datetime.utcnow() - age
        query = query.filter(Q(job_finished__gte = notBefore) | Q(job_finished__isnull = True))
    if hostname:
        query = query.filter(transferHost = hostname)
    
    query = query.values('file_state').annotate(number = Count('file_state'))
    
    for row in query:
        count[row['file_state'].lower()] = row['number']
    
    for s in filter(lambda s: s not in count, map(lambda s: s.lower(), STATES)):
        count[s] = 0
    
    # Couple of aggregations
    count['queued'] = count['submitted'] + count['ready']
    count['total'] = count['finished'] + count['failed'] + count['canceled']
    
    return count



def _getTransferAndSubmissionPerHost(timewindow):
    hostnames = []
    
    notBefore = datetime.utcnow() - timewindow
        
    submissions = {}
    query = Job.objects.values('submit_host')\
                       .filter(submit_time__gte = notBefore)\
                       .annotate(count = Count('submit_host'))
    for j in query:
        submissions[j['submit_host']] = j['count']
        hostnames.append(j['submit_host'])
        
    transfers = {}
    query = File.objects.values('transferHost')\
                        .filter(Q(job_finished__gte = notBefore) | Q(job_finished__isnull = True))\
                        .annotate(count = Count('transferHost'))
    for t in query:
        # Submitted do not have a transfer host!
        if t['transferHost']:
            transfers[t['transferHost']] = t['count']
            if t['transferHost'] not in hostnames:
                hostnames.append(t['transferHost'])
                
    active = {}
    query = File.objects.filter(file_state = 'ACTIVE').values('transferHost').annotate(count = Count('transferHost'))
    for t in query:
        active[t['transferHost']] = t['count']
        
    servers = {}
    for h in hostnames:
        if h in submissions: s = submissions[h]
        else:                s = 0
        if h in transfers:   t = transfers[h]
        else:                t = 0
        if h in active:      a = active[h]
        else:                a = 0
        servers[h] = {'submissions': s, 'transfers': t, 'active': a}
        
    return servers


def _getRetriedStats(timewindow, hostname):
    notBefore = datetime.utcnow() - timewindow
    
    retriedObjs = File.objects.filter(file_state__in = ['FAILED', 'FINISHED'], job_finished__gte = notBefore, retry__gt = 0)
    if hostname:
        retriedObjs = retriedObjs.filter(transferHost = hostname)
    retriedObjs = retriedObjs.values('file_state').annotate(number = Count('file_state'))
                          
    retried = {}
    for f in retriedObjs:
        retried[f['file_state'].lower()] = f['number']
    for s in [s for s in ['failed', 'finished'] if s not in retried]:
        retried[s] = 0
    
    return retried    


@jsonify
def overview(httpRequest):
    hostname = httpRequest.GET.get('hostname', None)
    
    overall = _getCountPerState(timedelta(hours = 24), hostname)
    lastHour = _getCountPerState(timedelta(hours = 1), hostname)
    retried = _getRetriedStats(timedelta(hours = 1), hostname)
        
    return {
       'lastday': overall,
       'lasthour': lastHour,
       'retried': retried
    }
    

def _getHostHeartBeatAndSegment():
    allHosts = Host.objects.order_by('hostname').filter(beat__gte = datetime.utcnow() - timedelta(minutes = 2))
    hostCount = len(allHosts)
    
    if hostCount == 0:
        return {}
    
    segmentSize = 0xFFFF / hostCount
    segmentMod  = 0xFFFF % hostCount
    
    hosts = {}
    index = 0
    for h in allHosts:
        hosts[h.hostname] = {
            'beat'    : h.beat,
            'index'   : index,
            'segment' : {
                'start': "%04X" % (segmentSize * index),
                'end'  : "%04X" % (segmentSize * (index + 1) - 1),
            }
        }
        
        if index == hostCount - 1:
            hosts[h.hostname]['segment']['end'] = "%04X" % (segmentSize * (index + 1) + segmentMod)
        
        index += 1       
    
    return hosts

@jsonify
def servers(httpRequest):
    beats     = _getHostHeartBeatAndSegment()
    transfers = _getTransferAndSubmissionPerHost(timedelta(hours = 12))
    
    servers = beats
    for k in servers:
        if k in transfers:
            servers[k].update(transfers.get(k))
        else:
            servers[k].update({'transfers': 0, 'active': 0, 'submissions': 0})
    
    return servers


@jsonify
def pervo(httpRequest):
    notBefore = datetime.utcnow() - timedelta(minutes = 30)
    
    query = File.objects.values('file_state', 'vo_name')\
                        .filter(Q(job_finished__gte = notBefore) | Q(job_finished__isnull = True))\
                        .annotate(count = Count('file_state'))
                        
    if httpRequest.GET.get('source_se', None):
        query = query.filter(source_se = httpRequest.GET['source_se'])
    if httpRequest.GET.get('dest_se', None):
        query = query.filter(source_se = httpRequest.GET['dest_se'])
                        
    query = query.order_by('file_state')
                        
    perVo = {}
    for voJob in query:
        vo = voJob['vo_name']
        if vo not in perVo:
            perVo[vo] = {}
        perVo[vo][voJob['file_state']] = voJob['count']

    return perVo


@jsonify
def profiling(httpRequest):
    profiling = {}
    
    info = ProfilingInfo.objects.all()
    if len(info) > 0:
        profiling['updated'] = info[0].updated
        profiling['period']  = info[0].period
    else:
        profiling['updated'] = False
        profiling['period']  = False
    
    
    profiles = ProfilingSnapshot.objects
    if not httpRequest.GET.get('showall', False):
        profiles = profiles.filter(cnt__gt = 0)
    
    (orderBy, orderDesc) = getOrderBy(httpRequest)
    if orderBy == 'scope':
        profiles = profiles.order_by(orderedField('scope', orderDesc))
    elif orderBy == 'called':
        profiles = profiles.order_by(orderedField('cnt', orderDesc))
    elif orderBy == 'aggregate':
        profiles = profiles.order_by(orderedField('total', orderDesc))
    elif orderBy == 'average':
        profiles = profiles.order_by(orderedField('average', orderDesc))
    elif orderBy == 'exceptions':
        profiles = profiles.order_by(orderedField('exceptions', orderDesc))
    else:    
        profiles = profiles.order_by('total')
    
    profiling['profiles'] = profiles.all()
    
    return profiling


def _slowEntry2Dict(queries):
    for q in queries:
        yield {
           'start_time':     q[0],
           'user_host':      q[1],
           'query_time':     q[2],
           'lock_time':      q[3],
           'rows_sent':      q[4],
           'rows_examined':  q[5],
           'db':             q[6],
           'last_insert_id': q[7],
           'insert_id':      q[8],
           'server_id':      q[9],
           'sql_text':       q[10]
        }


@jsonify
def slowQueries(httpRequest):
    engine = settings.DATABASES['default']['ENGINE']
    
    if engine.endswith('oracle'):
        return {'message': 'Not supported for Oracle'}
    
    try:
        dbName = settings.DATABASES['default']['NAME']
        cursor = connection.cursor()    
        cursor.execute("SELECT * FROM mysql.slow_log WHERE db = %s ORDER BY query_time DESC" , (dbName))
        return {'queries': _slowEntry2Dict(cursor.fetchall())}
    except DatabaseError, e:
        return {'message': str(e)}

