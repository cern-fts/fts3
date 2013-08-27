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
from ftsweb.models import Job, JobArchive
from jsonify import jsonify


@jsonify
def unique(httpRequest):
    notBefore = datetime.utcnow() - timedelta(hours = 12)
    
    vos          = Job.objects.values('vo_name').distinct()
    sources      = Job.objects.filter(Q(finish_time__isnull = True) | Q(finish_time__gte = notBefore))\
                      .values('source_se').distinct()
    destinations = Job.objects.filter(Q(finish_time__isnull = True) | Q(finish_time__gte = notBefore))\
                      .values('dest_se').distinct()
    
    return {
        'vos': [vo['vo_name'] for vo in vos.all()],
        'sources': [source['source_se'] for source in sources.all()],
        'destinations': [dest['dest_se'] for dest in destinations.all()]
    }

