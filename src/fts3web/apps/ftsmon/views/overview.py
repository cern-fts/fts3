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
from django.db.models import Count
import types
import sys

from ftsweb.models import Job, File, OptimizeActive
from jobs import setup_filters
from jsonify import jsonify
from util import get_order_by, paged
import settings


def _db_to_date():
    if settings.DATABASES['default']['ENGINE'] == 'django.db.backends.oracle':
        return 'TO_TIMESTAMP(%s, \'YYYY-MM-DD HH24:MI:SS.FF\')'
    elif settings.DATABASES['default']['ENGINE'] == 'django.db.backends.mysql':
        return 'STR_TO_DATE(%s, \'%%Y-%%m-%%d %%H:%%i:%%S\')'
    else:
        return '%s'


def _db_limit(sql, limit):
    if settings.DATABASES['default']['ENGINE'] == 'django.db.backends.oracle':
        return "SELECT * FROM (%s) WHERE rownum <= %d" % (sql, limit)
    elif settings.DATABASES['default']['ENGINE'] == 'django.db.backends.mysql':
        return sql + " LIMIT %d" % limit
    else:
        return sql


def _get_pair_limits(limits, source, destination):
    pair_limits = {'source': dict(), 'destination': dict()}
    for l in limits:
        if l[0] == source:
            if l[2]:
                pair_limits['source']['bandwidth'] = l[2]
            elif l[3]:
                pair_limits['source']['active'] = l[3]
        elif l[1] == destination:
            if l[2]:
                pair_limits['destination']['bandwidth'] = l[2]
            elif l[3]:
                pair_limits['destination']['active'] = l[3]
    if len(pair_limits['source']) == 0:
        del pair_limits['source']
    if len(pair_limits['destination']) == 0:
        del pair_limits['destination']
    return pair_limits


def _get_pair_udt(udt_pairs, source, destination):
    for u in udt_pairs:
        if udt_pairs[0] == source and udt_pairs[1] == destination:
            return True
        elif udt_pairs[0] == source and not udt_pairs[1]:
            return True
        elif not udt_pairs[0] and udt_pairs[1] == destination:
            return True
    return False


class OverviewExtended(object):
    """
    Wraps the return of overview, so when iterating, we can retrieve
    additional information.
    This way we avoid doing it for all items. Only those displayed will be queried
    (i.e. paging)
    """

    def __init__(self, not_before, objects, cursor):
        self.objects = objects
        self.not_before = not_before
        self.cursor = cursor

    def __len__(self):
        return len(self.objects)

    def _get_frequent_error(self, source, destination, vo):
        reason = File.objects.filter(source_se=source, dest_se=destination, vo_name=vo) \
            .filter(job_finished__gte=self.not_before, file_state='FAILED') \
            .values('reason').annotate(count=Count('reason')).values('reason', 'count').order_by('-count')[:1]
        if len(reason) > 0:
            return "[%(count)d] %(reason)s" % reason[0]
        else:
            return None

    def _get_fixed(self, source, destination):
        oa = OptimizeActive.objects.filter(source_se=source, dest_se=destination).values('fixed').all()
        if len(oa):
            oa = oa[0]
            return oa['fixed'] is not None and oa['fixed'].lower == 'on'
        return False

    def _get_average(self, source, destination, vo, active):
        if active > 0:
            query = """
            SELECT AVG(throughput), AVG(tx_duration) FROM t_file
            WHERE source_se = %s AND dest_se = %s AND vo_name = %s
                AND file_state in ('ACTIVE','FINISHED') AND throughput > 0
                AND (job_finished is NULL OR job_finished > """ + _db_to_date() + ")"
            self.cursor.execute(query, [source, destination, vo, self.not_before])
            result = self.cursor.fetchall()
            if len(result):
                avg_thr, avg_duration = result[0]
                total_thr = avg_thr * active if avg_thr is not None else None
                return total_thr, avg_duration
        return None, None

    def _get_job_state_count(self, source, destination, vo):
        states_count = Job.objects.filter(
            source_se=source, dest_se=destination, vo_name=vo, reuse_job__in=['Y', 'N'], job_finished__isnull=True
        ).values('job_state').annotate(count=Count('job_state')).values('job_state', 'count')
        states = dict()
        for row in states_count:
            states[row['job_state'].lower()] = row['count']
        return states

    def __getitem__(self, indexes):
        if isinstance(indexes, types.SliceType):
            return_list = self.objects[indexes]
            for item in return_list:
                item['most_frequent_error'] = self._get_frequent_error(item['source_se'], item['dest_se'],
                                                                       item['vo_name'])
                item['active_fixed'] = self._get_fixed(item['source_se'], item['dest_se'])
                item['current'], item['avg_duration'] = self._get_average(item['source_se'], item['dest_se'], item['vo_name'], item.get('active', 0))
                #item['job_states'] = self._get_job_state_count(item['source_se'], item['dest_se'], item['vo_name'])
            return return_list
        else:
            return self.objects[indexes]


@jsonify
def get_overview(http_request):
    filters = setup_filters(http_request)
    if filters['time_window']:
        not_before = datetime.utcnow() - timedelta(hours=filters['time_window'])
    else:
        not_before = datetime.utcnow() - timedelta(hours=1)

    cursor = connection.cursor()

    # Get all pairs first
    pairs_filter = ""
    se_params = []
    if filters['source_se']:
        pairs_filter += " AND source_se = %s "
        se_params.append(filters['source_se'])
    if filters['dest_se']:
        pairs_filter += " AND dest_se = %s "
        se_params.append(filters['dest_se'])
    if filters['vo']:
        pairs_filter += " AND vo_name = %s "
        se_params.append(filters['vo'])

    # Result
    triplets = dict()

    # Non terminal
    query = """
    SELECT COUNT(file_state) as count, file_state, source_se, dest_se, vo_name
    FROM t_file
    WHERE file_state in ('SUBMITTED', 'ACTIVE', 'STAGING', 'STARTED') %s
    GROUP BY file_state, source_se, dest_se, vo_name order by NULL
    """ % pairs_filter
    cursor.execute(query, se_params)
    for row in cursor.fetchall():
        triplet_key = (row[2], row[3], row[4])
        triplet = triplets.get(triplet_key, dict())

        triplet[row[1].lower()] = row[0]

        triplets[triplet_key] = triplet

    # Terminal
    query = """
    SELECT COUNT(file_state) as count, file_state, source_se, dest_se, vo_name
    FROM t_file
    WHERE file_state in ('FINISHED', 'FAILED', 'CANCELED') %s
        AND job_finished > %s
    GROUP BY file_state, source_se, dest_se, vo_name  order by NULL
    """ % (pairs_filter, _db_to_date())
    cursor.execute(query, se_params + [not_before])
    for row in cursor.fetchall():
        triplet_key = (row[2], row[3], row[4])
        triplet = triplets.get(triplet_key, dict())

        triplet[row[1].lower()] = row[0]

        triplets[triplet_key] = triplet

    # Limitations
    limit_query = "SELECT source_se, dest_se, throughput, active FROM t_optimize WHERE throughput IS NOT NULL or active IS NOT NULL"
    cursor.execute(limit_query)
    limits = cursor.fetchall()

    # UDT
    udt_query = "SELECT source_se, dest_se FROM t_optimize WHERE udt = 'on'"
    cursor.execute(udt_query)
    udt_pairs = cursor.fetchall()

    # Transform into a list
    objs = []
    for (triplet, obj) in triplets.iteritems():
        obj['source_se'] = triplet[0]
        obj['dest_se'] = triplet[1]
        obj['vo_name'] = triplet[2]
        if 'current' not in obj and 'active' in obj:
            obj['current'] = 0
        failed = obj.get('failed', 0)
        finished = obj.get('finished', 0)
        total = failed + finished
        if total > 0:
            obj['rate'] = (finished * 100.0) / total
        else:
            obj['rate'] = None
        # Append limit, if any
        obj['limits'] =_get_pair_limits(limits, triplet[0], triplet[1])
        # Mark UDT-enabled
        obj['udt'] = _get_pair_udt(udt_pairs, triplet[0], triplet[1])

        objs.append(obj)

    # Ordering
    (order_by, order_desc) = get_order_by(http_request)

    if order_by == 'active':
        sorting_method = lambda o: (o.get('active', 0), o.get('submitted', 0))
    elif order_by == 'finished':
        sorting_method = lambda o: (o.get('finished', 0), o.get('failed', 0))
    elif order_by == 'failed':
        sorting_method = lambda o: (o.get('failed', 0), o.get('finished', 0))
    elif order_by == 'canceled':
        sorting_method = lambda o: (o.get('canceled', 0), o.get('finished', 0))
    elif order_by == 'staging':
        sorting_method = lambda o: (o.get('staging', 0), o.get('started', 0))
    elif order_by == 'started':
        sorting_method = lambda o: (o.get('started', 0), o.get('staging', 0))
    elif order_by == 'rate':
        sorting_method = lambda o: (o.get('rate', 0), o.get('finished', 0))
    else:
        sorting_method = lambda o: (o.get('submitted', 0), o.get('active', 0))

    # Generate summary
    summary = {
        'submitted': sum(map(lambda o: o.get('submitted', 0), objs), 0),
        'active': sum(map(lambda o: o.get('active', 0), objs), 0),
        'finished': sum(map(lambda o: o.get('finished', 0), objs), 0),
        'failed': sum(map(lambda o: o.get('failed', 0), objs), 0),
        'canceled': sum(map(lambda o: o.get('canceled', 0), objs), 0),
        'current': sum(map(lambda o: o.get('current', 0), objs), 0),
        'staging': sum(map(lambda o: o.get('staging', 0), objs), 0),
        'started': sum(map(lambda o: o.get('started', 0), objs), 0),
    }
    if summary['finished'] > 0 or summary['failed'] > 0:
        summary['rate'] = (float(summary['finished']) / (summary['finished'] + summary['failed'])) * 100

    # Return
    return {
        'overview': paged(
            OverviewExtended(not_before, sorted(objs, key=sorting_method, reverse=order_desc), cursor=cursor),
            http_request
        ),
        'summary': summary
    }
