from datetime import datetime, timedelta
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.db.models import Q, Count, Avg
from django.shortcuts import render, redirect
from ftsweb.models import File


def showErrors(httpRequest):
    notbefore = datetime.utcnow() - timedelta(hours = 12)
    
    errors = File.objects.filter(reason__isnull = False, finish_time__gte = notbefore)\
                       .exclude(reason = '')\
                       .values('reason')\
                       .annotate(count = Count('reason'))\
                       .order_by('-count')\
                       
    # Render
    return render(httpRequest, 'errors.html',
                  {'errors': errors,
                   'request': httpRequest})


def transfersWithError(httpRequest, reason):
    if reason[0] == '"':
        reason = reason[1:-1]
    
    notbefore = datetime.utcnow() - timedelta(hours = 12)
    transfers = File.objects.filter(reason = reason, finish_time__gte = notbefore)\
                            .order_by('file_id')
    
    # Render
    return render(httpRequest, 'transfersWithError.html',
                  {'transfers': transfers,
                   'reason': reason,
                   'request': httpRequest
                  })
