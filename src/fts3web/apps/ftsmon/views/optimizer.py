from datetime import datetime, timedelta
from django.shortcuts import render, redirect
from django.db.models import Max, Avg, StdDev, Count, Min
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.http import Http404
from ftsweb.models import Optimize, OptimizerEvolution, File, Job
from ftsmon import forms

		

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
	
	# Paginate
	paginator = Paginator(optimizations, 50)
	try:
		if 'page' in httpRequest.GET: 
			optimizations = paginator.page(httpRequest.GET.get('page'))
		else:
			optimizations = paginator.page(1)
	except PageNotAnInteger:
		optimizations = paginator.page(1)
	except EmptyPage:
		optimizations = paginator.page(paginator.num_pages)
		
	extra_args = filterForm.args()	
	
	return render(httpRequest, 'optimizer/optimizer.html',
					{'request': httpRequest,
					'filterForm': filterForm,
					'optimizations': optimizations,
					'paginator': paginator,
					'extra_args': extra_args
					})



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
								 finish_time__gte = notBefore)
	
	fsizes = fsizes.aggregate(nfiles  = Count('file_id'),
							  biggest = Max('filesize'),
							  smallest = Min('filesize'),
							  average  = Avg('filesize'),
							  deviation = StdDev('filesize'))
	
	# Optimizer snapshot
	optimizerSnapshot = Optimize.objects.filter(source_se = source_se, dest_se = dest_se,
											   throughput__isnull = False,
											   datetime__gte = notBefore).order_by('-datetime')
											   
	# Optimizer evolution
	optimizerHistory = OptimizerEvolution.objects.filter(source_se = source_se, dest_se = dest_se,
														 datetime__gte = notBefore).order_by('-datetime').all()[:50]
	
	return render(httpRequest, 'optimizer/details.html',
					{'request': httpRequest,
					 'source_se': source_se,
					 'dest_se': dest_se,
					 'fsizes': fsizes,
					 'optimizerSnapshot': optimizerSnapshot,
					 'optimizerHistory': optimizerHistory
					})

