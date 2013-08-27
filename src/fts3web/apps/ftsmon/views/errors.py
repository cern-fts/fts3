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
from django.shortcuts import render, redirect
from ftsweb.models import File
from jsonify import jsonify_paged

@jsonify_paged
def showErrors(httpRequest):
    notbefore = datetime.utcnow() - timedelta(hours = 12)
    errors = File.objects.filter(reason__isnull = False, finish_time__gte = notbefore)\
                         .exclude(reason = '')\
                         .values('reason', 'source_se', 'dest_se')\
                         .annotate(count = Count('reason'))\
                         .order_by('-count')
    
    return errors
  
  
def placeholder(request):
    errors = File.objects.filter(reason__isnull = False, finish_time__gte = notbefore)\
                       .exclude(reason = '')\
                       .values('reason')\
                       .annotate(count = Count('reason'))\
                       .order_by('-count')\
                       
    return errors


@jsonify_paged
def transfersWithError(httpRequest):
    if 'reason' not in httpRequest.GET or not httpRequest.GET['reason']:
        return redirect('ftsmon.views.showErrors')
    
    reason = httpRequest.GET['reason']

    notbefore = datetime.utcnow() - timedelta(hours = 12)
    transfers = File.objects.filter(reason = reason, finish_time__gte = notbefore)
    
    if httpRequest.GET.get('source_se', None):
        transfers = transfers.filter(source_se = httpRequest.GET['source_se'])
    if httpRequest.GET.get('dest_se', None):
        transfers = transfers.filter(dest_se = httpRequest.GET['dest_se'])
    
    transfers = transfers.values('file_id', 'job_id', 'source_surl', 'dest_surl', 'job__vo_name')\
                         .order_by('file_id')
                            
    return transfers
