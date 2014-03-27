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
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.db.models import Q, Count, Avg
from django.http import Http404
from django.shortcuts import render, redirect
from ftsweb.models import File
from jsonify import jsonify_paged

_TIMEDELTA = timedelta(hours = 1)

@jsonify_paged
def showErrors(httpRequest):
    notBefore = datetime.utcnow() - _TIMEDELTA
    errors = File.objects.filter(file_state = 'FAILED', job_finished__gte = notBefore)

    if httpRequest.GET.get('source_se', None):
        errors = errors.filter(source_se = httpRequest.GET['source_se'])
    if httpRequest.GET.get('dest_se', None):
        errors = errors.filter(dest_se = httpRequest.GET['dest_se'])
    if httpRequest.GET.get('vo', None):
        errors = errors.filter(vo_name = httpRequest.GET['vo'])
    if httpRequest.GET.get('reason', None):
        errors = errors.filter(reason__icontains = httpRequest.GET['reason'])
                         
    errors = errors.values('source_se', 'dest_se')\
                         .annotate(count = Count('file_state'))\
                         .order_by('-count')

    return errors


@jsonify_paged
def errorsForPair(httpRequest):
    source_se = httpRequest.GET.get('source_se', None)
    dest_se   = httpRequest.GET.get('dest_se', None)
    reason    = httpRequest.GET.get('reason', None)
    
    if not source_se or not dest_se:
        raise Http404

    notBefore = datetime.utcnow() - _TIMEDELTA
    transfers = File.objects.filter(file_state = 'FAILED',
                                    job_finished__gte = notBefore,
                                    source_se = source_se, dest_se = dest_se)
    if reason:
        transfers = transfers.filter(reason__icontains = reason)
    
    transfers = transfers.values('vo_name', 'reason')
    
    transfers = transfers.annotate(count = Count('reason'))
                            
    return transfers.order_by('-count')
