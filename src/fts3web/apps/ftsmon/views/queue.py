from ftsweb.models import File
from django.db.models import Count
from django.core.paginator import Paginator
from django.shortcuts import render
from urllib import urlencode
from utils import getPage


def queueByPairs(httpRequest):
    pairs = File.objects.filter(file_state__in = ['SUBMITTED', 'READY'])
    pairs = pairs.values('source_se', 'dest_se', 'job__vo_name')
    pairs = pairs.annotate(ntransfers = Count('file_id'))
    pairs = pairs.order_by('source_se', 'dest_se', 'job__vo_name')
    
    paginator = Paginator(pairs, 50)
    return render(httpRequest, 'queue/queue.html',
                  {'pairsPerVo': getPage(paginator, httpRequest),
                   'paginator': paginator,
                   'request': httpRequest})



def queueDetailed(httpRequest):
    source_se = httpRequest.GET['source_se']
    dest_se   = httpRequest.GET['dest_se']
    vo_name   = httpRequest.GET['vo']
    
    transfers = File.objects.filter(file_state__in = ['SUBMITTED', 'READY'])
    transfers = transfers.filter(source_se = source_se, dest_se = dest_se, job__vo_name = vo_name)
    
    extra_args = urlencode({'source_se': source_se,
                            'dest_se': dest_se,
                            'vo': vo_name})
    
    paginator = Paginator(transfers, 50)
    return render(httpRequest, 'queue/details.html',
                  {'transfers': getPage(paginator, httpRequest),
                   'paginator': paginator,
                   'request': httpRequest,
                   'extra_args': extra_args})



def queue(httpRequest):
    get = httpRequest.GET
    if reduce(lambda a,b: a and b, map(lambda k: k in get, ['source_se', 'dest_se', 'vo'])):
        return queueDetailed(httpRequest)
    else:
        return queueByPairs(httpRequest)
