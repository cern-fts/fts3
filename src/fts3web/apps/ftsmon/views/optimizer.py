from django.shortcuts import render, redirect
from django.db.models import Max
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from fts3.models import Optimize
from ftsmon import forms


def optimizer(httpRequest):
	# Initialize forms
	filterForm    = forms.FilterForm(httpRequest.GET)
	
	# Query
	optimizations = Optimize.objects.filter(throughput__isnull = False)
	
	if filterForm.is_valid():
		if filterForm['source_se'].value():
			optimizations = optimizations.filter(source_se = filterForm['source_se'].value())
		if filterForm['dest_se'].value():
			optimizations = optimizations.filter(dest_se = filterForm['dest_se'].value())
	
	# Only max!   
	optimizations = optimizations.extra(where = [
	""" t_optimize.throughput = (SELECT MAX(O2.throughput) FROM t_optimize O2
	                                 WHERE O2.source_se = t_optimize.source_se AND
	                                       O2.dest_se = t_optimize.dest_se AND
	                                       O2.active = t_optimize.active)"""])
	
	optimizations = optimizations.order_by('source_se', 'dest_se', 'active')
	print str(optimizations.query)
	
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
	
	return render(httpRequest, 'optimizer.html',
					{'request': httpRequest,
					'filterForm': filterForm,
					'optimizations': optimizations,
					'paginator': paginator,
					'extra_args': extra_args
					})
