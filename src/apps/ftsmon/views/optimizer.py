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
from django.db import connection
from django.db.models import Sum, Q
from django.http import Http404

from ftsweb.models import OptimizerEvolution, OptimizerStreams, OptimizeActive
from authn import require_certificate
from jsonify import jsonify, jsonify_paged
from util import paged


@require_certificate
@jsonify_paged
def get_optimizer_pairs(http_request):
    # From the optimizer evolution
    from_optimizer = OptimizerEvolution.objects.filter(throughput__isnull=False)\
        .values('source_se', 'dest_se')

    if http_request.GET.get('source_se', None):
        from_optimizer = from_optimizer.filter(source_se=http_request.GET['source_se'])
    if http_request.GET.get('dest_se', None):
        from_optimizer = from_optimizer.filter(dest_se=http_request.GET['dest_se'])
    try:
        time_window = timedelta(hours=int(http_request.GET['time_window']))
    except:
        time_window = timedelta(minutes=30)

    not_before = datetime.utcnow() - time_window
    from_optimizer = from_optimizer.filter(datetime__gte=not_before)
    from_optimizer = from_optimizer.distinct()

    # Append configured fixed active for convenience
    from_fixed = OptimizeActive.objects
    if http_request.GET.get('source_se', None):
        from_fixed = from_fixed.filter(source_se=http_request.GET['source_se'])
    if http_request.GET.get('dest_se', None):
        from_fixed = from_fixed.filter(dest_se=http_request.GET['dest_se'])

    from_fixed = from_fixed.values('source_se', 'dest_se', 'fixed').distinct()

    return list(from_optimizer) + list(from_fixed)


class OptimizerAppendLimits(object):
    """
    Query for the limits if the branch is 10 (limited bandwidth)
    """

    def __init__(self, source_se, dest_se, evolution):
        self.source_se = source_se
        self.dest_se = dest_se
        self.evolution = evolution

    def __len__(self):
        return len(self.evolution)

    def __getitem__(self, index):
        entries = self.evolution[index]
        if isinstance(entries, list):
            for e in entries:
                if e['branch'] == 10:
                    e['bandwidth_limits'] = {
                        'source': self._get_source_limit(),
                        'destination': self._get_destination_limit()
                    }
        return entries

    def _get_source_limit(self):
        cursor = connection.cursor()
        cursor.execute("SELECT throughput FROM t_optimize WHERE source_se = %s AND throughput IS NOT NULL",
                       [self.source_se])
        result = cursor.fetchall()
        if len(result) < 1:
            return None
        else:
            return result[0][0]

    def _get_destination_limit(self):
        cursor = connection.cursor()
        cursor.execute("SELECT throughput FROM t_optimize WHERE dest_se = %s AND throughput IS NOT NULL",
                       [self.dest_se])
        result = cursor.fetchall()
        if len(result) < 1:
            return None
        else:
            return result[0][0]


@require_certificate
@jsonify
def get_optimizer_details(http_request):
    source_se = str(http_request.GET.get('source', None))
    dest_se = str(http_request.GET.get('destination', None))

    if not source_se or not dest_se:
        raise Http404

    try:
        time_window = timedelta(hours=int(http_request.GET['time_window']))
        not_before = datetime.utcnow() - time_window
    except:
        not_before = datetime.utcnow() - timedelta(minutes=30)

    optimizer = OptimizerEvolution.objects.filter(source_se=source_se, dest_se=dest_se)
    optimizer = optimizer.filter(datetime__gte=not_before)
    optimizer = optimizer.values('datetime', 'active', 'throughput', 'success', 'branch')
    optimizer = optimizer.order_by('-datetime')

    fixed = OptimizeActive.objects.filter(fixed='on', source_se=source_se, dest_se=dest_se)
    if len(fixed):
        fixed = fixed[0]
    else:
        fixed = None

    return {
        'evolution': paged(OptimizerAppendLimits(source_se, dest_se, optimizer), http_request),
        'fixed': fixed
    }


class AppendQuantile(object):
    """
    Compute the quantile for each stream
    """

    def __init__(self, streams, quantiles):
        self.streams = streams
        self.quantiles = quantiles
        self.n_quantiles = len(self.quantiles)

    def __len__(self):
        return len(self.streams)

    def __getitem__(self, index):
        for s in self.streams[index]:
            s.quantile = len(filter(lambda x: x < s.throughput, self.quantiles)) + 1
            s.quantiles = self.n_quantiles
            yield s


@require_certificate
@jsonify
def get_optimizer_streams(http_request):
    streams = OptimizerStreams.objects
    if http_request.GET.get('source_se', None):
        streams = streams.filter(source_se=http_request.GET['source_se'])
    if http_request.GET.get('dest_se', None):
        streams = streams.filter(dest_se=http_request.GET['dest_se'])
    streams = streams.order_by('-throughput')

    # Compute quantiles
    n_quantiles = 10
    rows = sorted(map(lambda s: s.throughput, streams))
    n_rows = len(rows)
    if n_rows:
        quantiles = map(
            lambda x: rows[int(x) - 1],
            map(lambda y: n_rows * (float(y)/n_quantiles), range(1, n_quantiles + 1))
        )
    else:
        quantiles = [0] * n_quantiles

    return paged(AppendQuantile(streams, quantiles), http_request)
