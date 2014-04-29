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
from django.db.models import Count, Sum, Q, Avg
from django.core.paginator import Paginator
from django.shortcuts import render
from ftsweb.models import File
from jobs import setupFilters
from jsonify import jsonify
from urllib import urlencode
from util import getOrderBy, paged
import types

import settings

def _db_to_date():
    if settings.DATABASES['default']['ENGINE'] == 'django.db.backends.oracle':
        return 'TO_DATE(%s, \'YYYY-MM-DD HH24:MI:SS\')'
    elif settings.DATABASES['default']['ENGINE'] == 'django.db.backends.mysql':
        return 'STR_TO_DATE(%s, \'%%Y-%%m-%%d %%H:%%i:%%S\')'
    else:
        return '%s'

def _get_bandwidth_limit(bw_limits, source, destination):
    limits = {}
    for l in bw_limits:
        if l[0] == source: 
            limits['source'] = l[2]
        elif l[1] == destination:
            limits['destination'] = l[2]
    return limits


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
        avg_duration = File.objects.filter(source_se = source, dest_se = destination, vo_name = vo)\
            .filter(job_finished__gte = self.not_before, file_state ='FINISHED')\
            .aggregate(Avg('tx_duration'))
        return avg_duration['tx_duration__avg']

    def _get_avg_queued(self, source, destination, vo):
        if settings.DATABASES['default']['ENGINE'] == 'django.db.backends.oracle':
            avg_queued_query = """SELECT AVG(EXTRACT(HOUR FROM time_diff) * 3600 + EXTRACT(MINUTE FROM time_diff) * 60 + EXTRACT(SECOND FROM time_diff))
                                  FROM (SELECT (t_file.start_time - t_job.submit_time) AS time_diff
                                      FROM t_file, t_job
                                      WHERE t_job.job_id = t_file.job_id AND
                                            t_file.source_se = %s AND t_file.dest_se = %s AND t_job.vo_name = %s AND
                                            t_job.job_finished IS NULL AND t_file.job_finished IS NULL AND
                                            t_file.start_time IS NOT NULL
                                   )"""
        else:
            avg_queued_query = """SELECT AVG(TIMESTAMPDIFF(SECOND, t_job.submit_time, t_file.start_time)) FROM t_file, t_job
                                  WHERE t_job.job_id = t_file.job_id AND
                                        t_file.source_se = %s AND t_file.dest_se = %s AND t_job.vo_name = %s AND
                                        t_job.job_finished IS NULL AND t_file.job_finished IS NULL AND
                                        t_file.start_time IS NOT NULL
                                  ORDER BY t_file.start_time DESC LIMIT 5"""

        cursor = connection.cursor()
        cursor.execute(avg_queued_query, (source, destination, vo))
        avg = cursor.fetchall()
        if len(avg) > 0 and avg[0][0] is not None:
            return int(avg[0][0])
        else:
            return None

    def _get_frequent_error(self, source, destination, vo):
        reason = File.objects.filter(source_se = source, dest_se = destination, vo_name = vo)\
            .filter(job_finished__gte = self.not_before, file_state ='FAILED')\
            .values('reason').annotate(count = Count('reason')).values('reason', 'count').order_by('-count')
        if len(reason) > 0:
            return "[%(count)d] %(reason)s" % reason[0]
        else:
            return None

    def __getitem__(self, indexes):
        if isinstance(indexes, types.SliceType):
            return_list = self.objects[indexes]
            for item in return_list:
                item['avg_duration'] = self._get_avg_duration(item['source_se'], item['dest_se'], item['vo_name'])
                item['avg_queued'] = self._get_avg_queued(item['source_se'], item['dest_se'], item['vo_name'])
                item['most_frequent_error'] = self._get_frequent_error(item['source_se'], item['dest_se'], item['vo_name'])
            return return_list
        else:
            return self.objects[indexes]


@jsonify
def overview(httpRequest):
    filters    = setupFilters(httpRequest)
    if filters['time_window']:
        notBefore  = datetime.utcnow() - timedelta(hours = filters['time_window'])
    else:
        notBefore  = datetime.utcnow() - timedelta(hours = 1)
    throughputWindow = datetime.utcnow() - timedelta(seconds = 5)

    query = """
    SELECT source_se, dest_se, vo_name, file_state, count(file_id),
           SUM(CASE WHEN job_finished >= %s OR job_finished IS NULL THEN throughput ELSE 0 END)
    FROM t_file
    WHERE (job_finished IS NULL OR job_finished >= %s)
        AND file_state IN ('SUBMITTED', 'ACTIVE','FINISHED','FAILED','CANCELED')
    """ % (_db_to_date(), _db_to_date())
    
    params = [throughputWindow.strftime('%Y-%m-%d %H:%M:%S'),
              notBefore.strftime('%Y-%m-%d %H:%M:%S')]
    
    # Filtering
    if filters['vo']:
        query += ' AND vo_name = %s '
        params.append(filters['vo'])
    if filters['source_se']:
        query += ' AND source_se = %s '
        params.append(filters['source_se'])
    if filters['dest_se']:
        query += ' AND dest_se = %s '
        params.append(filters['dest_se'])
    
    query += ' GROUP BY source_se, dest_se, vo_name, file_state '
    query += ' ORDER BY NULL'

    cursor = connection.cursor()
    cursor.execute(query, params)
    
    # Limitations
    limit_query = "SELECT source_se, dest_se, throughput FROM t_optimize WHERE throughput IS NOT NULL"
    limit_cursor = connection.cursor()
    limit_cursor.execute(limit_query)
    limits = limit_cursor.fetchall()
    
    # Need to group by pairs :(
    grouped = {}
    for p in cursor.fetchall():
        triplet = p[0:3]
        if triplet not in grouped:
            grouped[triplet] = {}
        grouped[triplet][p[3].lower()] = p[4]
        if p[5]:
            grouped[triplet]['current'] = grouped[triplet].get('current', 0) + p[5]
            
    # And transform into a list
    objs = []
    for (triplet, obj) in grouped.iteritems():
        obj['source_se'] = triplet[0]
        obj['dest_se']   = triplet[1]
        obj['vo_name']   = triplet[2]
        if 'current' not in obj and 'active' in obj:
            obj['current'] = 0
        failed = obj.get('failed', 0)
        finished = obj.get('finished', 0)
        total = failed + finished
        if total > 0:
            obj['rate'] = (finished * 100.0) / total
        # Append limit, if any
        obj['bandwidth_limit'] = _get_bandwidth_limit(limits, triplet[0], triplet[1])
        objs.append(obj)
    
    # Ordering
    (orderBy, orderDesc) = getOrderBy(httpRequest)

    if orderBy == 'active':
        sortingMethod = lambda o: (o.get('active', 0), o.get('submitted', 0))
    elif orderBy == 'finished':
        sortingMethod = lambda o: (o.get('finished', 0), o.get('failed', 0))
    elif orderBy == 'failed':
        sortingMethod = lambda o: (o.get('failed', 0), o.get('finished', 0))
    elif orderBy == 'canceled':
        sortingMethod = lambda o: (o.get('canceled', 0), o.get('finished', 0))
    elif orderBy == 'throughput':
        sortingMethod = lambda o: (o.get('current', 0), o.get('active', 0))
    elif orderBy == 'rate':
        sortingMethod = lambda o: (o.get('rate', 0), o.get('finished', 0))
    else:
        sortingMethod = lambda o: (o.get('submitted', 0), o.get('active', 0))
        
    # Generate summary
    summary = {
        'submitted': reduce(lambda a, b: a + b, map(lambda o: o.get('submitted', 0), objs), 0),
        'active': reduce(lambda a, b: a + b, map(lambda o: o.get('active', 0), objs), 0),
        'finished': reduce(lambda a, b: a + b, map(lambda o: o.get('finished', 0), objs), 0),
        'failed': reduce(lambda a, b: a + b, map(lambda o: o.get('failed', 0), objs), 0),
        'canceled': reduce(lambda a, b: a + b, map(lambda o: o.get('canceled', 0), objs), 0),
        'current': reduce(lambda a, b: a + b, map(lambda o: o.get('current', 0), objs), 0),
    }
    if summary['finished'] > 0 or summary['failed'] > 0:
        summary['rate'] = (float(summary['finished']) / (summary['finished'] + summary['failed'])) * 100

    # Return
    return {
        'overview': paged(OverviewExtended(notBefore, sorted(objs, key = sortingMethod, reverse = orderDesc)), httpRequest),
        'summary': summary
    }
