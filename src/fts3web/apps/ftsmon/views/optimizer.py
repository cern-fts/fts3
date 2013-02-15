from django.shortcuts import render, redirect
from fts3.models import Optimize
from ftsmon import forms

def optimizer(httpRequest):
    # Initialize forms
    filterForm    = forms.FilterForm(httpRequest.GET)
    
    # Query
    optimizations = Optimize.objects.filter(throughput__isnull = False)
    
    if filterForm.is_valid():
        if filterForm['vo'].value():
            optimizations = optimizations.filter(file__job__vo_name = filterForm['vo'].value())
        if filterForm['source_se'].value():
            optimizations = optimizations.filter(source_se = filterForm['source_se'].value())
        if filterForm['dest_se'].value():
            optimizations = optimizations.filter(dest_se = filterForm['dest_se'].value())
    
    
    return render(httpRequest, 'optimizer.html',
              {'request': httpRequest,
               'filterForm': filterForm,
               'optimizations': optimizations,
               })
