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
from django.db.models import Q, Count, Sum
from django.db.utils import DatabaseError

from ftsweb.models import Job, File, Host
from ftsweb.models import ProfilingSnapshot, ProfilingInfo, Turl
from ftsweb.models import ACTIVE_STATES, STATES
from jsonify import jsonify, jsonify_paged
from util import get_order_by, ordered_field
import settings


def _get_count_per_state(age, hostname):
    count = {}

    not_before = datetime.utcnow() - age
    for state in STATES:
        query = File.objects
        if state in ACTIVE_STATES:
            query = query.filter(job_finished__isnull=True)
        else:
            query = query.filter(job_finished__gte=not_before)
        if hostname:
            query = query.filter(transferHost=hostname)
        query = query.filter(file_state=state)

        count[state.lower()] = query.count()

    # Couple of aggregations
    count['queued'] = count['submitted'] + count['ready']
    count['total'] = count['finished'] + count['failed'] + count['canceled']

    return count


def _get_transfer_and_submission_per_host(timewindow):
    servers = {}

    not_before = datetime.utcnow() - timewindow
    hosts = Host.objects.filter().values('hostname').distinct()

    for host in map(lambda h: h['hostname'], hosts):
        submissions = Job.objects.filter(submit_time__gte=not_before, submit_host=host).count()
        transfers = File.objects.filter(job_finished__gte=not_before, transferHost=host).count()
        actives = File.objects.filter(file_state='ACTIVE', transferHost=host).count()
        staging =  File.objects.filter(file_state='STAGING', transferHost=host).count()
        started =  File.objects.filter(file_state='STARTED', transferHost=host).count()
        servers[host] = {
            'submissions': submissions,
            'transfers': transfers,
            'active': actives,
            'staging': staging,
            'started': started
        }

    return servers


def _get_retried_stats(timewindow, hostname):
    not_before = datetime.utcnow() - timewindow

    retried_objs = File.objects.filter(file_state__in=['FAILED', 'FINISHED'], job_finished__gte=not_before, retry__gt=0)
    if hostname:
        retried_objs = retried_objs.filter(transferHost=hostname)
    retried_objs = retried_objs.values('file_state').annotate(number=Count('file_state'))

    retried = {}
    for f in retried_objs:
        retried[f['file_state'].lower()] = f['number']
    for s in [s for s in ['failed', 'finished'] if s not in retried]:
        retried[s] = 0

    return retried


@jsonify
def get_overview(http_request):
    hostname = http_request.GET.get('hostname', None)

    last_hour = _get_count_per_state(timedelta(hours=1), hostname)
    retried = _get_retried_stats(timedelta(hours=1), hostname)

    return {
        'lasthour': last_hour,
        'retried': retried
    }


# noinspection PyTypeChecker
def _get_host_service_and_segment():
    service_names = map(lambda s: s['service_name'], Host.objects.values('service_name').distinct().all())

    last_expected_beat = datetime.utcnow() - timedelta(minutes=2)

    host_map = dict()
    for service in service_names:
        hosts = Host.objects.filter(service_name=service).values('hostname', 'beat', 'drain').order_by('hostname').all()
        running = filter(lambda h: h['beat'] >= last_expected_beat, hosts)

        running_count = len(running)

        if running_count > 0:
            segment_size = 0xFFFF / running_count
            segment_remaining = 0xFFFF % running_count

            index = 0
            for host in hosts:
                hostname = host['hostname']

                if hostname not in host_map:
                    host_map[hostname] = dict()

                if hostname in [h['hostname'] for h in running]:
                    host_map[hostname][service] = {
                        'status': 'running',
                        'start': "%04X" % (segment_size * index),
                        'end': "%04X" % (segment_size * (index + 1) - 1),
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
def get_servers(http_request):
    segments = _get_host_service_and_segment()
    transfers = _get_transfer_and_submission_per_host(timedelta(hours=1))

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
def get_pervo(http_request):
    not_before = datetime.utcnow() - timedelta(minutes=30)

    # Terminal first
    terminal = File.objects.values('file_state', 'vo_name') \
        .filter(job_finished__gte=not_before).annotate(count=Count('file_state'))

    if http_request.GET.get('source_se', None):
        terminal = terminal.filter(source_se=http_request.GET['source_se'])
    if http_request.GET.get('dest_se', None):
        terminal = terminal.filter(source_se=http_request.GET['dest_se'])

    per_vo = {}
    for row in terminal:
        vo = row['vo_name']
        if vo not in per_vo:
            per_vo[vo] = {}
        per_vo[vo][row['file_state'].lower()] = row['count']

    # Non terminal, one by one
    # See ticket #1083
    for state in ['ACTIVE', 'SUBMITTED']:
        non_terminal = File.objects.values('vo_name').filter(Q(job_finished__isnull=True) & Q(file_state=state))
        if http_request.GET.get('source_se', None):
            non_terminal = non_terminal.filter(source_se=http_request.GET['source_se'])
        if http_request.GET.get('dest_se', None):
            non_terminal = non_terminal.filter(source_se=http_request.GET['dest_se'])
        non_terminal = non_terminal.annotate(count=Count('file_state'))

        for row in non_terminal:
            vo = row['vo_name']
            if vo not in per_vo:
                per_vo[vo] = {}
            per_vo[vo][state.lower()] = row['count']

    return per_vo


class CalculateVolume(object):
    def __init__(self, triplets, not_before):
        self.triplets = triplets
        self.not_before = not_before

    def __len__(self):
        return len(self.triplets)

    def __getitem__(self, indexes):
        if not isinstance(indexes, slice):
            indexes = [indexes]
        for triplet in self.triplets[indexes]:
            pair_volume = File.objects.filter(
                source_se=triplet['source_se'],
                dest_se=triplet['dest_se'],
                vo_name=triplet['vo'],
                file_state='FINISHED',
                job_finished__lt=self.not_before
            ).aggregate(vol=Sum('filesize'))
            triplet['volume'] = pair_volume['vol']
            yield triplet


@jsonify_paged
def get_transfer_volume(http_request):
    try:
        time_window = timedelta(hours=int(http_request.GET['time_window']))
    except:
        time_window = timedelta(hours=1)
    not_before = datetime.utcnow() - time_window

    if http_request.GET.get('vo', None):
        vos = [http_request.GET['vo']]
    else:
        vos = [vo['vo_name'] for vo in Job.objects.values('vo_name').distinct().all()]

    triplets = []
    for vo in vos:
        pairs = File.objects.values('source_se', 'dest_se').distinct()
        if http_request.GET.get('source_se', None):
            pairs = pairs.filter(source_se=http_request.GET['source_se'])
        if http_request.GET.get('dest_se', None):
            pairs = pairs.filter(dest_se=http_request.GET['dest_se'])
        pairs = pairs.filter(vo_name=vo, file_state='FINISHED', job_finished__gte=not_before)

        for pair in pairs:
            source = pair['source_se']
            dest = pair['dest_se']
            triplets.append({
                'source_se': source,
                'dest_se': dest,
                'vo': vo
            })

    # Trick to calculate the sum only for those that are visible
    return CalculateVolume(triplets, not_before)


@jsonify_paged
def get_turls(http_request):
    try:
        time_window = timedelta(hours=int(http_request.GET['time_window']))
    except:
        time_window = timedelta(hours=1)
    not_before = datetime.utcnow() - time_window

    turls = Turl.objects.filter(datetime__gte = not_before)
    if http_request.GET.get('source_se'):
        turls = turls.filter(source_surl = http_request.GET['source_se'])
    if http_request.GET.get('dest_se'):
        turls = turls.filter(destin_surl = http_request.GET['dest_se'])

    (order_by, order_desc) = get_order_by(http_request)
    if order_by == 'throughput':
        turls = turls.order_by(ordered_field('throughput', order_desc))
    elif order_by == 'finish':
        turls = turls.order_by(ordered_field('finish', order_desc))
    elif order_by == 'fail':
        turls = turls.order_by(ordered_field('fail', order_desc))
    else:
        turls = turls.order_by('throughput')

    return turls.all()


@jsonify
def get_profiling(http_request):
    profiling = {}

    info = ProfilingInfo.objects.all()
    if len(info) > 0:
        profiling['updated'] = info[0].updated
        profiling['period'] = info[0].period
    else:
        profiling['updated'] = False
        profiling['period'] = False

    profiles = ProfilingSnapshot.objects
    if not http_request.GET.get('showall', False):
        profiles = profiles.filter(cnt__gt=0)

    (order_by, order_desc) = get_order_by(http_request)
    if order_by == 'scope':
        profiles = profiles.order_by(ordered_field('scope', order_desc))
    elif order_by == 'called':
        profiles = profiles.order_by(ordered_field('cnt', order_desc))
    elif order_by == 'aggregate':
        profiles = profiles.order_by(ordered_field('total', order_desc))
    elif order_by == 'average':
        profiles = profiles.order_by(ordered_field('average', order_desc))
    elif order_by == 'exceptions':
        profiles = profiles.order_by(ordered_field('exceptions', order_desc))
    else:
        profiles = profiles.order_by('total')

    profiling['profiles'] = profiles.all()

    return profiling


def _slow_entry_to_dict(queries):
    for q in queries:
        yield {
            'start_time': q[0],
            'user_host': q[1],
            'query_time': q[2],
            'lock_time': q[3],
            'rows_sent': q[4],
            'rows_examined': q[5],
            'db': q[6],
            'last_insert_id': q[7],
            'insert_id': q[8],
            'server_id': q[9],
            'sql_text': q[10]
        }


@jsonify
def get_slow_queries(http_request):
    engine = settings.DATABASES['default']['ENGINE']

    if engine.endswith('oracle'):
        return {'message': 'Not supported for Oracle'}

    try:
        dbname = settings.DATABASES['default']['NAME']
        cursor = connection.cursor()
        cursor.execute("SELECT * FROM mysql.slow_log WHERE db = %s ORDER BY query_time DESC", [dbname])
        return {'queries': _slow_entry_to_dict(cursor.fetchall())}
    except DatabaseError:
        return {'message': 'Could not execute the query'}
