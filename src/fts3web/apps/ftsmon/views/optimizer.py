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
	""" "T_OPTIMIZE"."THROUGHPUT" = (SELECT MAX("O2"."THROUGHPUT") FROM T_OPTIMIZE O2
	                                 WHERE "O2"."SOURCE_SE" = "T_OPTIMIZE"."SOURCE_SE" AND
	                                       "O2"."DEST_SE" = "T_OPTIMIZE"."DEST_SE" AND
	                                       "O2"."ACTIVE" = "T_OPTIMIZE"."ACTIVE")"""])
	
	optimizations = optimizations.order_by('source_se', 'dest_se', 'active')
	
	
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
