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
from django.db.models import Count, Avg
import types
import sys

from ftsweb.models import File, OptimizeActive
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

    def __init__(self, not_before, objects):
        self.objects = objects
        self.not_before = not_before

    def __len__(self):
        return len(self.objects)

    def _get_avg_duration(self, source, destination, vo):
        avg_duration = File.objects.filter(source_se=source, dest_se=destination, vo_name=vo) \
            .filter(job_finished__gte=self.not_before, file_state='FINISHED') \
            .aggregate(Avg('tx_duration'))
        return avg_duration['tx_duration__avg']

    def _get_frequent_error(self, source, destination, vo):
        reason = File.objects.filter(source_se=source, dest_se=destination, vo_name=vo) \
            .filter(job_finished__gte=self.not_before, file_state='FAILED') \
            .values('reason').annotate(count=Count('reason')).values('reason', 'count').order_by('-count')[:1]
        if len(reason) > 0:
            return "[%(count)d] %(reason)s" % reason[0]
        else:
            return None

    def _get_active_fixed(self, source, dest):
        oa = OptimizeActive.objects.filter(source_se=source, dest_se=dest)
        if len(oa):
            if oa[0].fixed == 'on':
                return True
            else:
                return False
        else:
            return False

    def __getitem__(self, indexes):
        if isinstance(indexes, types.SliceType):
            return_list = self.objects[indexes]
            for item in return_list:
                item['avg_duration'] = self._get_avg_duration(item['source_se'], item['dest_se'], item['vo_name'])
                item['most_frequent_error'] = self._get_frequent_error(item['source_se'], item['dest_se'],
                                                                       item['vo_name'])
                item['active_fixed'] = self._get_active_fixed(item['source_se'], item['dest_se'])
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

    # Get all vos
    if filters['vo']:
        all_vos = [filters['vo']]
    else:
        cursor.execute("SELECT DISTINCT vo_name FROM t_job")
        all_vos = [row[0] for row in cursor.fetchall()]

    # Get all pairs first
    pairs_query = "SELECT source_se, dest_se FROM t_optimize_active WHERE datetime >= %s " % _db_to_date()
    pairs_params = [not_before]
    if filters['source_se']:
        pairs_query += "  AND source_se = %s"
        pairs_params.append(filters['source_se'])
    if filters['dest_se']:
        pairs_query += " AND dest_se = %s"
        pairs_params.append(filters['dest_se'])
    cursor.execute(pairs_query, pairs_params)
    all_pairs = cursor.fetchall()

    triplets = {}
    for (source, dest) in all_pairs:
        for vo in all_vos:
            # Make sure the combination of source, dest and vo has actually been used
            cursor.execute(
                _db_limit(
                    """SELECT file_id FROM t_file
                    WHERE source_se = %s AND dest_se = %s AND vo_name = %s
                          AND file_state IN ('FAILED', 'FINISHED', 'CANCELED', 'SUBMITTED', 'ACTIVE')""",
                    1
                ),
                [source, dest, vo]
            )
            triplet_exists = len(cursor.fetchall()) > 0
            if triplet_exists:
                triplet_key = (source, dest, vo)
                triplet = triplets.get(triplet_key, dict())

                # File count
                cursor.execute(
                    "SELECT file_state, COUNT(file_state) FROM t_file "
                    "WHERE source_se = %%s AND dest_se = %%s AND vo_name = %%s"
                    "      AND (job_finished > %s OR job_finished IS NULL) "
                    "      AND file_state <> 'NOT_USED' "
                    "GROUP BY file_state ORDER BY NULL" % _db_to_date(),
                    [source, dest, vo, not_before]
                )
                for row in cursor.fetchall():
                    triplet[row[0].lower()] = row[1]

                # Total files
                total_files = sum(triplet.values())
                if total_files > 0:
                    # Throughput
                    if triplet.get('active') > 0:
                        cursor.execute(
                            "SELECT AVG(throughput) FROM t_file "
                            "WHERE source_se = %s AND dest_se=%s "
                            "      AND vo_name=%s AND file_state='FINISHED' AND throughput > 0"
                            "      AND job_finished > %s",
                            [source, dest, vo, not_before]
                        )
                        avg_thr = cursor.fetchall()
                        if len(avg_thr) and avg_thr[0][0]:
                            triplet['current'] = avg_thr[0][0] * triplet.get('active')

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
    elif order_by == 'throughput':
        if order_desc:
            # NULL current first (so when reversing, they are last)
            sorting_method = lambda o: (o.get('current', None), o.get('active', 0))
        else:
             # NULL current last
            sorting_method = lambda o: (o.get('current', sys.maxint), o.get('active', 0))
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
    }
    if summary['finished'] > 0 or summary['failed'] > 0:
        summary['rate'] = (float(summary['finished']) / (summary['finished'] + summary['failed'])) * 100

    # Return
    return {
        'overview': paged(
            OverviewExtended(not_before, sorted(objs, key=sorting_method, reverse=order_desc)),
            http_request
        ),
        'summary': summary
    }
