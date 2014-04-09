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

import simplejson
from datetime import datetime, timedelta
from django.db.models import Q
from django.http import HttpResponse
from ftsweb.models import Job, File, Host
from jsonify import jsonify

@jsonify
def activities(httpRequest):
    activities = File.objects.values('activity').distinct()
    return [row['activity'] for row in activities]

@jsonify
def sources(httpRequest):
    notBefore = datetime.utcnow() - timedelta(hours = 12)
    sources = Job.objects.filter(Q(job_finished__isnull = True) | Q(job_finished__gte = notBefore))\
                 .values('source_se').distinct()
    return [row['source_se'] for row in sources]

@jsonify
def destinations(httpRequest):
    notBefore = datetime.utcnow() - timedelta(hours = 12)
    sources = Job.objects.filter(Q(job_finished__isnull = True) | Q(job_finished__gte = notBefore))\
                 .values('dest_se').distinct()
    return [row['dest_se'] for row in sources]

@jsonify
def vos(httpRequest):
    vos = Job.objects.values('vo_name').distinct()
    return [row['vo_name'] for row in vos]


@jsonify
def hostnames(httpRequest):
    notBefore = datetime.utcnow() - timedelta(hours = 12)
    hosts = Host.objects.values('hostname').filter(beat__gte = notBefore).distinct()
    return [h['hostname'] for h in hosts]