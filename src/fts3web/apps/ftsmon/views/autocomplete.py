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

import datetime
import simplejson
from django.db.models import Q
from django.http import HttpResponse
from ftsweb.models import Job, JobArchive



def uniqueSources(httpRequest, archive):
    if archive:
        model = JobArchive
    else:
        model = Job

    query = model.objects.values('source_se').distinct('source_se')
    if 'term' in httpRequest.GET and str(httpRequest.GET['term']) != '':
        query = query.filter(source_se__icontains = httpRequest.GET['term'])

    ses = []
    for se in query:
        ses.append(se['source_se'])
    return HttpResponse(simplejson.dumps(ses), mimetype='application/json')



def uniqueDestinations(httpRequest, archive):
    if archive:
        model = JobArchive
    else:
        model = Job
    
    query = model.objects.values('dest_se').distinct('dest_se')
    if 'term' in httpRequest.GET and str(httpRequest.GET['term']) != '':
        query = query.filter(dest_se__icontains = httpRequest.GET['term'])
    
    ses = []
    for se in query:
        ses.append(se['dest_se'])
    return HttpResponse(simplejson.dumps(ses), mimetype='application/json')



def uniqueVos(httpRequest, archive):
    if archive:
        model = JobArchive
    else:
        model = Job
    
    query = model.objects.values('vo_name').distinct('vo_name')
    if 'term' in httpRequest.GET and str(httpRequest.GET['term']) != '':
        query = query.filter(vo_name__icontains = httpRequest.GET['term'])
    
    vos = []
    for vo in query:
        vos.append(vo['vo_name'])

    return HttpResponse(simplejson.dumps(vos), mimetype='application/json')
