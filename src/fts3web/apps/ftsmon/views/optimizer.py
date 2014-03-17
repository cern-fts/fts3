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
from django.db import connection
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
    filter_form = forms.FilterForm(httpRequest.GET)
    time_window = timedelta(minutes = 30)
    
    # Query
    pairs = OptimizerEvolution.objects.filter(throughput__isnull = False)\
                        .values('source_se', 'dest_se')
    if filter_form.is_valid():
        if filter_form['source_se'].value():
            pairs = pairs.filter(source_se = filter_form['source_se'].value())
        if filter_form['dest_se'].value():
            pairs = pairs.filter(dest_se = filter_form['dest_se'].value())
        if filter_form['time_window'].value():
            try:
                time_window = timedelta(hours = int(filter_form['time_window'].value()))
            except:
                pass

    not_before = datetime.utcnow() - time_window
    pairs = pairs.filter(datetime__gte = not_before)

    return pairs.distinct()


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
                         'source': self._getSourceLimit(),
                         'destination': self._getDestinationLimit()
                    }
        return entries
    
    def _getSourceLimit(self):
        cursor = connection.cursor()
        cursor.execute("SELECT throughput FROM t_optimize WHERE source_se = %s", [self.source_se])
        result = cursor.fetchall()
        if len(result) < 1:
            return None
        else:
            return result[0][0]
        
    def _getDestinationLimit(self):
        cursor = connection.cursor()
        cursor.execute("SELECT throughput FROM t_optimize WHERE dest_se = %s", [self.dest_se])
        result = cursor.fetchall()
        if len(result) < 1:
            return None
        else:
            return result[0][0]


@jsonify_paged
def optimizerDetailed(httpRequest):
    source_se = str(httpRequest.GET.get('source', None))
    dest_se   = str(httpRequest.GET.get('destination', None))

    if not source_se or not dest_se:
        raise Http404

    try:
        time_window = httpRequest.GET['time_window']
        not_before = datetime.utcnow() - timedelta(hours = int(time_window))
    except:
        not_before = datetime.utcnow() - timedelta(minutes = 30)

    optimizer = OptimizerEvolution.objects.filter(source_se = source_se, dest_se = dest_se)
    optimizer = optimizer.filter(datetime__gte = not_before)
    optimizer = optimizer.values('datetime', 'active', 'throughput', 'success', 'branch')
    optimizer = optimizer.order_by('-datetime')
    return OptimizerAppendLimits(source_se, dest_se, optimizer)
