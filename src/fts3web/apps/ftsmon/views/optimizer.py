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
from django.db.models import Max, Avg, StdDev, Count, Min
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.http import Http404
from ftsweb.models import Optimize, OptimizerEvolution, OptimizeActive
from ftsweb.models import File, Job
from ftsmon import forms
from jsonify import jsonify, jsonify_paged
from util import getOrderBy, orderedField
		

@jsonify_paged
def optimizer(httpRequest):
	# Initialize forms
	filterForm    = forms.FilterForm(httpRequest.GET)
	
	# Query
	optimizations = Optimize.objects.filter(throughput__isnull = False)\
						.values('source_se', 'dest_se')

	hours = 48
	if filterForm.is_valid():
		if filterForm['source_se'].value():
			optimizations = optimizations.filter(source_se = filterForm['source_se'].value())
		if filterForm['dest_se'].value():
			optimizations = optimizations.filter(dest_se = filterForm['dest_se'].value())
		if filterForm['time_window'].value():
			hours = int(filterForm['time_window'].value())
			
	notBefore = datetime.utcnow() - timedelta(hours = hours)
	optimizations = optimizations.filter(datetime__gte = notBefore)
	
	# Only max!   
	optimizations = optimizations.annotate(max_throughput = Max('throughput'),
										   avg_throughput = Avg('throughput'),
										   std_deviation  = StdDev('throughput'))
	optimizations = optimizations.order_by('source_se', 'dest_se')
	
	return optimizations


@jsonify
def optimizerDetailed(httpRequest):
	hours = 48	
	try:
		source_se = str(httpRequest.GET['source'])
		dest_se   = str(httpRequest.GET['destination'])
	except:
		source_se = None
		dest_se   = None
	finally:
		if not source_se or not dest_se:
			raise Http404
		
	try:
		if 'time_window' in httpRequest.GET:
			hours = int(httpRequest.GET['time_window'])
	except:
		pass
	notBefore = datetime.utcnow() - timedelta(hours = hours)
	
	# File sizes
	fsizes = File.objects.filter(source_se = source_se, dest_se = dest_se,
								 file_state = 'FINISHED',
								 job_finished__gte = notBefore)
	
	fsizes = fsizes.aggregate(nfiles  = Count('file_id'),
							  biggest = Max('filesize'),
							  smallest = Min('filesize'),
							  average  = Avg('filesize'),
							  deviation = StdDev('filesize'))
	
	# Optimizer snapshot
	optimizerSnapshot = Optimize.objects.filter(source_se = source_se, dest_se = dest_se,
											   throughput__isnull = False,
											   datetime__gte = notBefore)\
									    .distinct().order_by('-datetime')
											   
	# Optimizer evolution
	optimizerHistory = OptimizerEvolution.objects.filter(source_se = source_se, dest_se = dest_se,
														 datetime__gte = notBefore).order_by('-datetime').all()[:50]
														 
	return {
		'fsizes': fsizes,
		'snapshot': optimizerSnapshot,
		'history': optimizerHistory
	}


@jsonify_paged
def optimizerActive(httpRequest):
	active = OptimizeActive.objects
	
	source_se = httpRequest.GET.get('source_se', None)
	dest_se   = httpRequest.GET.get('dest_se', None)
	
	if source_se:
		active = active.filter(source_se = source_se)
	if dest_se:
		active = active.filter(dest_se = dest_se)
	
	(orderBy, orderDesc) = getOrderBy(httpRequest)
	if orderBy == 'active':
		active = active.order_by(orderedField('active', orderDesc))
	else:
		active = active.order_by('source_se', 'dest_se')
	
	return active
