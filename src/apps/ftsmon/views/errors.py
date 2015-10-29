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
from django.db.models import Count
from django.http import Http404
from ftsweb.models import File
from authn import require_certificate
from jsonify import jsonify, jsonify_paged
from util import paged


@require_certificate
@jsonify_paged
def get_errors(http_request):
    try:
        time_window = timedelta(hours=int(http_request.GET['time_window']))
    except:
        time_window = timedelta(hours=1)

    not_before = datetime.utcnow() - time_window
    errors = File.objects.filter(file_state='FAILED', job_finished__gte=not_before)

    if http_request.GET.get('source_se', None):
        errors = errors.filter(source_se=http_request.GET['source_se'])
    if http_request.GET.get('dest_se', None):
        errors = errors.filter(dest_se=http_request.GET['dest_se'])
    if http_request.GET.get('vo', None):
        errors = errors.filter(vo_name=http_request.GET['vo'])
    if http_request.GET.get('reason', None):
        errors = errors.filter(reason__icontains=http_request.GET['reason'])

    errors = errors.values('source_se', 'dest_se') \
        .annotate(count=Count('file_state')) \
        .order_by('-count', '-job_finished')
    # Fetch all first to avoid 'count' query
    return list(errors.all())


@require_certificate
@jsonify
def get_errors_for_pair(http_request):
    source_se = http_request.GET.get('source_se', None)
    dest_se = http_request.GET.get('dest_se', None)
    reason = http_request.GET.get('reason', None)

    if not source_se or not dest_se:
        raise Http404

    try:
        time_window = timedelta(hours=int(http_request.GET['time_window']))
    except:
        time_window = timedelta(hours=1)

    not_before = datetime.utcnow() - time_window
    transfers = File.objects.filter(file_state='FAILED',
                                    job_finished__gte=not_before,
                                    source_se=source_se, dest_se=dest_se)
    if reason:
        transfers = transfers.filter(reason__icontains=reason)

    transfers = transfers.values('vo_name', 'reason')
    transfers = transfers.annotate(count=Count('reason'))
    transfers = transfers.order_by('-count', '-job_finished')
    # Trigger query to fetch all
    transfers = list(transfers)

    # Count by error type
    classification = dict()
    for t in transfers:
        words = t['reason'].split()
        if len(words):
            scope = words[0]
            if scope.isupper():
                classification[scope] = classification.get(scope, 0) + t['count']

    return {
        'errors': paged(transfers, http_request),
        'classification': classification
    }
