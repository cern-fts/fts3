from django.shortcuts import render, redirect
from django.db.models import Max, Avg
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from fts3.models import Optimize
from ftsmon import forms


class OptimizerGenerator(object):
	def __init__(self, optimizations):
		self.optimizations = optimizations
		
	def __len__(self):
		return len(self.optimizations)
	
	def __getitem__(self, index):
		items = self.optimizations[index]
		for item in items:
			item.update(Optimize.objects.filter(source_se = item['source_se'],
												dest_se = item['dest_se'],
												active = item['max_active'])\
										.values('nostreams', 'timeout', 'buffer').all()[0])
		return items
		

def optimizer(httpRequest):
	# Initialize forms
	filterForm    = forms.FilterForm(httpRequest.GET)
	
	# Query
	optimizations = Optimize.objects.filter(throughput__isnull = False)\
						.values('source_se', 'dest_se')
	
	if filterForm.is_valid():
		if filterForm['source_se'].value():
			optimizations = optimizations.filter(source_se = filterForm['source_se'].value())
		if filterForm['dest_se'].value():
			optimizations = optimizations.filter(dest_se = filterForm['dest_se'].value())
	
	# Only max!   
	optimizations = optimizations.annotate(max_active     = Max('active'),
										   max_throughput = Max('throughput'),
										   avg_throughput = Avg('throughput'))
	optimizations = optimizations.order_by('source_se', 'dest_se')
	
	# Wrap with a generator so the number of streams and timeout can be retrieved
	optimizations = OptimizerGenerator(optimizations)
	
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
