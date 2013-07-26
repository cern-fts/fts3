# Copyright notice:
# Copyright © Members of the EMI Collaboration, 2010.
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
from utils import getPage


def showErrors(httpRequest):
    notbefore = datetime.utcnow() - timedelta(hours = 12)
    
    errors = File.objects.filter(reason__isnull = False, finish_time__gte = notbefore)\
                       .exclude(reason = '')\
                       .values('reason')\
                       .annotate(count = Count('reason'))\
                       .order_by('-count')\
                       
    # Render
    return render(httpRequest, 'errors/errorCount.html',
                  {'errors': errors,
                   'request': httpRequest})


def transfersWithError(httpRequest):
    if 'reason' not in httpRequest.GET or \
        not httpRequest.GET['reason']:
        return redirect('ftsmon.views.showErrors')
    
    reason = httpRequest.GET['reason']

    notbefore = datetime.utcnow() - timedelta(hours = 12)
    transfers = File.objects.filter(reason = reason, finish_time__gte = notbefore)\
                            .order_by('file_id')
                            
    paginator = Paginator(transfers, 50)
    
    # Render
    return render(httpRequest, 'errors/transfersWithError.html',
                  {'transfers': getPage(paginator, httpRequest),
                   'paginator': paginator,
                   'reason': reason,
                   'request': httpRequest
                  })
