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
from django.db.models import Count, Sum, Q
from django.core.paginator import Paginator
from django.shortcuts import render
from ftsmon import forms
from ftsweb.models import File
from jobs import setupFilters
from jsonify import jsonify_paged
from urllib import urlencode


@jsonify_paged
def overview(httpRequest):
    filterForm = forms.FilterForm(httpRequest.GET)
    filters    = setupFilters(filterForm)
    notBefore  = datetime.utcnow() - timedelta(hours = 1)
    
    pairs = File.objects\
                .filter(Q(finish_time__isnull = True) | Q(finish_time__gte = notBefore))
    
    if filters['vo']:
        pairs = pairs.filter(job__vo_name = filters['vo'])
    if filters['source_se']:
        pairs = pairs.filter(source_se = filters['source_se'])
    if filters['dest_se']:
        pairs = pairs.filter(dest_se = filters['dest_se'])
    
    pairs = pairs.values('source_se', 'dest_se', 'job__vo_name', 'file_state')
    pairs = pairs.annotate(ntransfers = Count('file_id'), throughput = Sum('throughput'))
    
    # Need to group by pairs :(
    grouped = {}
    for p in pairs:
        triplet = (p['source_se'], p['dest_se'], p['job__vo_name'])
        if triplet not in grouped:
            grouped[triplet] = {}
        grouped[triplet][p['file_state'].lower()] = p['ntransfers']
        if p['file_state'] == 'ACTIVE':
            grouped[triplet]['current'] = grouped[triplet].get('current', 0) + p['throughput']
        
    # And transform into a list
    objs = []
    for (triplet, obj) in grouped.iteritems():
        obj['source_se'] = triplet[0]
        obj['dest_se']   = triplet[1]
        obj['vo_name']   = triplet[2]
        objs.append(obj)
    
    # Ordering
    orderBy   = httpRequest.GET.get('orderby', None)
    orderDesc = False
    if orderBy and orderBy[0] == '-':
        orderBy = orderBy[1:]
        orderDesc = True

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

