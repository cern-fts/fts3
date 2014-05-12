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
from django.db.models import Q, Count, Avg, Sum
from django.db.utils import DatabaseError
from ftsweb.models import Job, File, Host
from ftsweb.models import ProfilingSnapshot, ProfilingInfo, OptimizeActive
from ftsweb.models import ACTIVE_STATES, FILE_TERMINAL_STATES, STATES
from jsonify import jsonify, jsonify_paged
from util import getOrderBy, orderedField
import settings


def _getCountPerState(age, hostname):
    count = {}
    
    notBefore = datetime.utcnow() - age
    for state in STATES:
        query = File.objects
        if state in ACTIVE_STATES:
            query = query.filter(job_finished__isnull = True)
        else:
            query = query.filter(job_finished__gte = notBefore)
        if hostname:
            query = query.filter(transferHost = hostname)
        query = query.filter(file_state = state)
        
        count[state.lower()] = query.count()

    # Couple of aggregations
    count['queued'] = count['submitted'] + count['ready']
    count['total'] = count['finished'] + count['failed'] + count['canceled']
    
    return count


def _getTransferAndSubmissionPerHost(timewindow):
    servers = {}
    
    notBefore = datetime.utcnow() - timewindow
    hosts = Host.objects.filter().values('hostname').distinct()

    for host in map(lambda h: h['hostname'], hosts):
        submissions = Job.objects.filter(submit_time__gte = notBefore, submit_host = host).count()
        transfers = File.objects.filter(job_finished__gte = notBefore, transferHost = host).count()
        actives = File.objects.filter(file_state = 'ACTIVE', transferHost = host).count()
        servers[host] = {
             'submissions': submissions,
             'transfers': transfers,
             'active': actives
        }

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
    
    lastHour = _getCountPerState(timedelta(hours = 1), hostname)
    retried = _getRetriedStats(timedelta(hours = 1), hostname)
        
    return {
       'lasthour': lastHour,
       'retried': retried
    }


def _getHostServiceAndSegment():
    service_names = map(lambda s: s['service_name'], Host.objects.values('service_name').distinct().all())
    
    last_expected_beat = datetime.utcnow() - timedelta(minutes = 2)

    host_map = dict()
    for service in service_names:
        hosts = Host.objects.filter(service_name = service).values('hostname', 'beat', 'drain').order_by('hostname').all()
        running = filter(lambda h: h['beat'] >= last_expected_beat, hosts)
        
        host_count = len(hosts)
        running_count = len(running)
        
        if running_count > 0:
            segment_size = 0xFFFF / running_count
            segment_remaining = 0xFFFF % running_count
            
            segments = dict()
            index = 0
            for host in hosts:
                hostname = host['hostname']

                if hostname not in host_map:
                    host_map[hostname] = dict()

                if hostname in [h['hostname'] for h in running]:
                    host_map[hostname][service] = {
                        'status': 'running',
                        'start': "%04X" % (segment_size * index),
                        'end' : "%04X" % (segment_size * (index + 1) - 1),
                        'drain': host['drain'],
                        'beat': host['beat']
                    }
                    index += 1
                    if index == running_count:
                        host_map[hostname][service]['end'] = "%04X" % (segment_size * index + segment_remaining)
                else:
                    host_map[hostname][service] = {
                        'drain': host['drain'],
                        'beat': host['beat'],
                        'status': 'down'
                    }
        else:
            for host in hosts:
                hostname = host['hostname']
                if hostname not in host_map:
                    host_map[hostname] = dict()
                host_map[hostname][service] = {
                    'status': 'down',
                    'beat': host['beat'],
                    'drain': host['drain']
                }
        
    return host_map


@jsonify
def servers(httpRequest):
    segments  = _getHostServiceAndSegment()
    transfers = _getTransferAndSubmissionPerHost(timedelta(hours = 1))
    
    hosts = segments.keys()

    servers = dict()
    for host in hosts:
        servers[host] = dict()
        if host in transfers:
            servers[host].update(transfers.get(host))
        else:
            servers[host].update({'transfers': 0, 'active': 0, 'submissions': 0})
            
        servers[host]['services'] = segments[host]
                
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


class CalculateVolume(object):
    def __init__(self, triplets, notBefore):
        self.triplets = triplets
        self.notBefore = notBefore

    def __len__(self):
        return len(self.triplets)

    def __getitem__(self, indexes):
        if not isinstance(indexes, slice):
            indexes = [indexes]
        for triplet in self.triplets[indexes]:
            pairVolume = File.objects.filter(
                 source_se = triplet['source_se'],
                 dest_se = triplet['dest_se'],
                 vo_name = triplet['vo'],
                 file_state = 'FINISHED',
                 job_finished__lt = self.notBefore
            ).aggregate(vol = Sum('filesize'))
            triplet['volume'] = pairVolume['vol']
            yield triplet

@jsonify_paged
def transferVolume(httpRequest):
    try:
        timeWindow = timedelta(hours = int(httpRequest.GET['time_window']))
    except:
        timeWindow = timedelta(hours = 1)
    notBefore = datetime.utcnow() - timeWindow
    
    if httpRequest.GET.get('vo', None):
        vos= [httpRequest.GET['vo']]
    else:
        vos = [vo['vo_name'] for vo in Job.objects.values('vo_name').distinct().all()]

    triplets = []
    for vo in vos:       
        pairs = File.objects.values('source_se', 'dest_se').distinct()
        if httpRequest.GET.get('source_se', None):
            pairs = pairs.filter(source_se = httpRequest.GET['source_se'])
        if httpRequest.GET.get('dest_se', None):
            pairs = pairs.filter(dest_se = httpRequest.GET['dest_se'])
        pairs = pairs.filter(vo_name = vo, file_state = 'FINISHED', job_finished__lt = notBefore)

        for pair in pairs:
            source = pair['source_se']
            dest = pair['dest_se']
            triplets.append({
                'source_se': source,
                'dest_se': dest,
                'vo': vo
            })
    
    # Trick to calculate the sum only for those that are visible
    return CalculateVolume(triplets, notBefore)


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
        return {'message': 'Could not execute the query'}

