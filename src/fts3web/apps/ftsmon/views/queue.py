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

from ftsweb.models import File
from django.db.models import Count
from django.core.paginator import Paginator
from django.shortcuts import render
from ftsmon import forms
from jobs import setupFilters
from urllib import urlencode
from utils import getPage


def queueByPairs(httpRequest):
    filterForm = forms.FilterForm(httpRequest.GET)
    filters    = setupFilters(filterForm)
    
    pairs = File.objects.filter(file_state__in = ['SUBMITTED', 'READY'])
    
    if filters['vo']:
        pairs = pairs.filter(job__vo_name = filters['vo'])
    if filters['source_se']:
        pairs = pairs.filter(source_se = filters['source_se'])
    if filters['dest_se']:
        pairs = pairs.filter(dest_se = filters['dest_se'])
    
    pairs = pairs.values('source_se', 'dest_se', 'job__vo_name')
    pairs = pairs.annotate(ntransfers = Count('file_id'))
    pairs = pairs.order_by('source_se', 'dest_se', 'job__vo_name')
    
    paginator = Paginator(pairs, 50)
    return render(httpRequest, 'queue/queue.html',
                  {'pairsPerVo': getPage(paginator, httpRequest),
                   'paginator': paginator,
                   'request': httpRequest,
                   'filterForm': filterForm})



def queueDetailed(httpRequest):
    filterForm = forms.FilterForm(httpRequest.GET)
    filters    = setupFilters(filterForm)
    
    transfers = File.objects.filter(file_state__in = ['SUBMITTED', 'READY'])
    transfers = transfers.filter(source_se = filters['source_se'],
                                 dest_se = filters['dest_se'],
                                 job__vo_name = filters['vo'])
    
    print transfers.query
    
    extra_args = urlencode({'source_se': filters['source_se'],
                            'dest_se': filters['dest_se'],
                            'vo': filters['vo']})
    
    paginator = Paginator(transfers, 50)
    return render(httpRequest, 'queue/details.html',
                  {'transfers': getPage(paginator, httpRequest),
                   'paginator': paginator,
                   'request': httpRequest,
                   'extra_args': extra_args,
                   'filters': filters,
                   'filterForm': filterForm})



def queue(httpRequest):
    get = httpRequest.GET
    
    filteredFields = ['source_se', 'dest_se', 'vo']
    
    if reduce(lambda a,b: a and b, map(lambda k: k in get and get[k], filteredFields)):
        return queueDetailed(httpRequest)
    else:
        return queueByPairs(httpRequest)
