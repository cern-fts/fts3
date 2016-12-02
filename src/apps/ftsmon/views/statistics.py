# Copyright notice:
#
# Copyright (C) CERN 2013-2015
#
# Copyright (C) Members of the EMI Collaboration, 2010-2013.
#   See www.eu-emi.eu for details on the copyright holders
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
from ftsweb.models import ACTIVE_STATES, STATES
from authn import require_certificate
from jsonify import jsonify, jsonify_paged, as_json
from slsfy import slsfy, slsfy_error
from util import get_order_by, ordered_field, log_link
import settings


def _get_count_per_state(age, hostname):
    count = {}

    not_before = datetime.utcnow() - age
    for state in STATES:
        query = File.objects
        if state not in ACTIVE_STATES:
            query = query.filter(finish_time__gte=not_before)
        if hostname:
            query = query.filter(transfer_host=hostname)
        query = query.filter(file_state=state)

        count[state.lower()] = query.count()

    # Couple of aggregations
    count['queued'] = count['submitted'] + count['ready']
    count['total'] = count['finished'] + count['failed'] + count['canceled']

    return count


def _get_transfer_and_submission_per_host(timewindow, segments):
    servers = {}

    not_before = datetime.utcnow() - timewindow
    hosts = Host.objects.filter().values('hostname').distinct()

    for host in map(lambda h: h['hostname'], hosts):
        submissions = Job.objects.filter(submit_time__gte=not_before, submit_host=host).count()
        transfers = File.objects.filter(finish_time__gte=not_before, transfer_host=host).count()
        actives = File.objects.filter(file_state='ACTIVE', transfer_host=host).count()
        staging =  File.objects.filter(file_state='STAGING', staging_host=host).count()
        started =  File.objects.filter(file_state='STARTED', staging_host=host).count()
        servers[host] = {
            'submissions': submissions,
            'transfers': transfers,
            'active': actives,
            'staging': staging,
            'started': started,
        }

    return servers


def _get_retried_stats(timewindow, hostname):
    not_before = datetime.utcnow() - timewindow

    retried_objs = File.objects.filter(file_state__in=['FAILED', 'FINISHED'], finish_time__gte=not_before, retry__gt=0)
    if hostname:
        retried_objs = retried_objs.filter(transferHost=hostname)
    retried_objs = retried_objs.values('file_state').annotate(number=Count('file_state'))

    retried = {}
    for f in retried_objs:
        retried[f['file_state'].lower()] = f['number']
    for s in [s for s in ['failed', 'finished'] if s not in retried]:
        retried[s] = 0

    return retried


@require_certificate
@jsonify
def get_overview(http_request):
    try:
        time_window = timedelta(hours=int(http_request.GET['time_window']))
    except:
        time_window = timedelta(hours=1)

    hostname = http_request.GET.get('hostname', None)

    last_hour = _get_count_per_state(time_window, hostname)
    retried = _get_retried_stats(time_window, hostname)

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
        running = map(lambda h: h['hostname'], filter(lambda h: h['beat'] >= last_expected_beat, hosts))

        running_count = len(running)

        if running_count > 0:
            segment_size = 0xFFFF / running_count
            segment_remaining = 0xFFFF % running_count

            index = 0
            for host in hosts:
                hostname = host['hostname']

                if hostname not in host_map:
                    host_map[hostname] = dict()

                if hostname in running:
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


def _query_worrying_level(time_elapsed, state):
    """
    Gives a "worriness" level to a query
    For instance, long times waiting for something to happen is bad
    Very long times sending is bad too
    Return a value between 0 and 1 rating the "worrying level"
    See http://dev.mysql.com/doc/refman/5.7/en/general-thread-states.html
    """
    state_lower = state.lower()

    if state in ('creating sort index', 'sorting result'):
        max_time = 60
    elif state in ('creating table', 'creating tmp table', 'removing tmp table'):
        max_time = 180
    elif state == 'copying to tmp table on disk':
        max_time = 60
    elif state in ('executing', 'preparing'):
        max_time = 300
    elif state == 'logging slow query':
        return 0.5
    elif state_lower == 'sending data':
        max_time = 600
    elif state_lower in ('sorting for group', 'sorting for order'):
        max_time = 60
    elif state_lower.startswith('waiting'):
        max_time = 600
    else:
        return 0

    if time_elapsed > max_time:
        return 1
    else:
        return float(time_elapsed) / max_time


def _get_database():
    cursor = connection.cursor()
    cursor.execute("""
      SELECT Id, Host, Command, Time, State, Info
      FROM INFORMATION_SCHEMA.PROCESSLIST
      WHERE State != ''
      ORDER BY State ASC, TIME DESC
    """)
    for row in cursor.fetchall():
        query = str(row[5])
        # Try not to show user dn on the query
        if query:
            where_index = query.lower().find('where')
            user_dn_index = query[where_index:].find('user_dn')
            if user_dn_index > -1:
                query = query[0:where_index + user_dn_index + 7] + ' ....'
        yield row[0], row[1], row[2], row[3], row[4],\
              query, _query_worrying_level(row[3], row[4])


def _get_server(time_window):
    segments = _get_host_service_and_segment()
    transfers = _get_transfer_and_submission_per_host(time_window, segments)

    hosts = segments.keys()

    servers = dict()
    for host in hosts:
        servers[host] = dict()
        if host in transfers:
            servers[host].update(transfers[host])
        else:
            servers[host].update({'transfers': 0, 'active': 0, 'submissions': 0})

        servers[host]['services'] = segments[host]
    return servers


# This one does not require certificate, so the Service Level can be still queried
def get_servers(http_request):
    try:
        time_window = timedelta(hours=int(http_request.GET['time_window']))
    except:
        time_window = timedelta(hours=1)

    format = http_request.GET.get('format', None)
    try:
        if format == 'sls':
            # For SLS, poll the DB load first. If it is way too high, do not bother querying for the servers
            database = _get_database()
            waiting_times = map(
                lambda d: d[3],
                filter(lambda d: d[4].lower().startswith('waiting'), database)
            )
            if len(waiting_times):
                avg_waiting_time = reduce(int.__add__, waiting_times, 0) / len(waiting_times)
            else:
                avg_waiting_time = 0

            if avg_waiting_time > 120 and len(waiting_times) > 5:
                servers = dict()
            else:
                servers = _get_server(time_window)

            return  slsfy(servers, id_tail='Server Info')
        else:
            return as_json(_get_server(time_window))
    except Exception, e:
        if format == 'sls':
            return slsfy_error(str(e), id_tail='Server Info')
        else:
            return as_json(dict(exception=str(e)))


@require_certificate
@jsonify
def get_database(http_request):
    return _get_database()


@require_certificate
@jsonify
def get_pervo(http_request):
    try:
        time_window = timedelta(hours=int(http_request.GET['time_window']))
    except:
        time_window = timedelta(hours=1)

    not_before = datetime.utcnow() - time_window

    # Terminal first
    terminal = File.objects.values('file_state', 'vo_name') \
        .filter(finish_time__gte=not_before).annotate(count=Count('file_state'))

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
        non_terminal = File.objects.values('vo_name').filter(Q(file_state=state))
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
                finish_time__gte=self.not_before
            ).aggregate(vol=Sum('filesize'))
            triplet['volume'] = pair_volume['vol']
            yield triplet


@require_certificate
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
        pairs = pairs.filter(vo_name=vo, file_state='FINISHED', finish_time__gte=not_before)

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

