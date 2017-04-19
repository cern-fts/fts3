# Copyright notice:
#
# Copyright (C) CERN 2013-2015
#
# Copyright (C) Members of the EMI Collaboration, 2010-2013.
#   See www.eu-emi.eu for details on the copyright holders
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
from django.views.decorators.cache import cache_page

from ftsweb.models import Job, File, Host
from jsonify import jsonify


@cache_page(1200)
@jsonify
def get_unique_activities(http_request):
    activities = File.objects.values('activity', 'vo_name').distinct()
    per_vo = {}
    for row in activities:
        if row['vo_name'] not in per_vo:
            per_vo[row['vo_name']] = []
    per_vo[row['vo_name']].append(row['activity'])
    return per_vo


@cache_page(300)
@jsonify
def get_unique_sources(http_request):
    sources = Job.objects.values('source_se').distinct()
    return filter(lambda h: h, map(lambda r: r['source_se'], sources))


@cache_page(300)
@jsonify
def get_unique_destinations(http_request):
    # Since there is no index prefix with dest_se, get unique pairs and then do the 'uniqueness' ourselves
    pairs = Job.objects.values('source_se', 'dest_se').distinct()
    return set(filter(lambda h: h, map(lambda r: r['dest_se'], pairs)))


@cache_page(300)
@jsonify
def get_unique_vos(http_request):
    vos = Job.objects.values('vo_name').distinct()
    return [row['vo_name'] for row in vos]


@cache_page(1200)
@jsonify
def get_unique_hostnames(http_request):
    not_before = datetime.utcnow() - timedelta(hours=1)
    hosts = Host.objects.values('hostname').filter(beat__gte=not_before).distinct()
    return [h['hostname'] for h in hosts]
