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
from django.db.models import Count, Sum, Q
from django.core.paginator import Paginator
from django.shortcuts import render
from ftsmon import forms
from ftsweb.models import File
from jobs import setupFilters
from jsonify import jsonify_paged, jsonify
from urllib import urlencode
from util import getOrderBy

import settings

def _db_to_date():
    if settings.DATABASES['default']['ENGINE'] == 'django.db.backends.oracle':
        return 'TO_DATE(%s, \'YYYY-MM-DD HH24:MI:SS\')'
    elif settings.DATABASES['default']['ENGINE'] == 'django.db.backends.mysql':
        return 'STR_TO_DATE(%s, \'%%Y-%%m-%%d %%H:%%i:%%S\')'
    else:
        return '%s'

@jsonify_paged
def overview(httpRequest):
    filterForm = forms.FilterForm(httpRequest.GET)
    filters    = setupFilters(filterForm)
    notBefore  = datetime.utcnow() - timedelta(hours = 1)
    throughputWindow = datetime.utcnow() - timedelta(seconds = 15)

    query = """
    SELECT source_se, dest_se, vo_name, file_state, count(file_id),
           SUM(CASE WHEN job_finished >= %s OR job_finished IS NULL THEN throughput ELSE 0 END)
    FROM t_file
    WHERE (job_finished IS NULL OR job_finished >= %s)
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

    cursor = connection.cursor()
    cursor.execute(query, params)
    
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
        failed = obj.get('failed', 0) + obj.get('canceled', 0)
        finished = obj.get('finished', 0)
        total = failed + finished
        if total > 0:
            obj['rate'] = (finished * 100.0) / total
        objs.append(obj)
    
    # Ordering
    (orderBy, orderDesc) = getOrderBy(httpRequest)

    if orderBy == 'active':
        sortingMethod = lambda o: (o.get('active', 0), o.get('submitted', 0))
    elif orderBy == 'finished':
        sortingMethod = lambda o: (o.get('finished', 0), o.get('failed', 0))
    elif orderBy == 'failed':
        sortingMethod = lambda o: (o.get('failed', 0), o.get('finished', 0))
    elif orderBy == 'throughput':
        sortingMethod = lambda o: (o.get('current', 0), o.get('active', 0))
    else:
        sortingMethod = lambda o: (o.get('submitted', 0), o.get('active', 0))

    return sorted(objs, key = sortingMethod, reverse = orderDesc)
