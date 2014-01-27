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
from django.db.models import Max, Avg, StdDev, Count, Min
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.http import Http404
from ftsweb.models import OptimizerEvolution
from ftsweb.models import File, Job
from ftsmon import forms
from jsonify import jsonify, jsonify_paged
from util import getOrderBy, orderedField
        

@jsonify_paged
def optimizer(httpRequest):
    # Initialize forms
    filterForm    = forms.FilterForm(httpRequest.GET)
    
    # Query
    pairs = OptimizerEvolution.objects.filter(throughput__isnull = False)\
                        .values('source_se', 'dest_se')
    if filterForm.is_valid():
        if filterForm['source_se'].value():
            pairs = pairs.filter(source_se = filterForm['source_se'].value())
        if filterForm['dest_se'].value():
            pairs = pairs.filter(dest_se = filterForm['dest_se'].value())

    notBefore = datetime.utcnow() - timedelta(hours = 12)
    pairs = pairs.filter(datetime__gte = notBefore)

    return pairs.distinct()


@jsonify_paged
def optimizerDetailed(httpRequest):
    source_se = str(httpRequest.GET.get('source', None))
    dest_se   = str(httpRequest.GET.get('destination', None))

    if not source_se or not dest_se:
        raise Http404
        
    notBefore = datetime.utcnow() - timedelta(hours = 1)

    optimizer = OptimizerEvolution.objects.filter(source_se = source_se, dest_se = dest_se)
    optimizer = optimizer.filter(datetime__gte = notBefore)
    optimizer = optimizer.values('datetime', 'nostreams', 'active', 'throughput')
    optimizer = optimizer.order_by('-datetime')

    return optimizer
