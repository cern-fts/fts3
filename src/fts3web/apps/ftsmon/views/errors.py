from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.db.models import Q, Count, Avg
from django.shortcuts import render, redirect
from fts.models import File


def showErrors(httpRequest):
    errors = File.objects.filter(reason__isnull = False)\
                       .exclude(reason = '')\
                       .values('reason')\
                       .annotate(count = Count('reason'))\
                       .order_by('-count')\
                       
    # Render
    return render(httpRequest, 'errors.html',
                  {'errors': errors,
                   'request': httpRequest})


def transfersWithError(httpRequest):
    if 'reason' not in httpRequest.POST:
        return redirect(to = showErrors)
    
    reason = httpRequest.POST['reason']
    
    transfers = File.objects.filter(reason = reason)
    
    # Render
    return render(httpRequest, 'transfersWithError.html',
                  {'transfers': transfers,
                   'reason': reason,
                   'request': httpRequest
                  })
