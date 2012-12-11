import datetime
import simplejson
from django.db.models import Q
from django.http import HttpResponse
from fts3.models import Job, File



def uniqueSources(httpRequest):
    notBefore = datetime.datetime.now() - datetime.timedelta(hours = 12)
    
    query = Job.objects.values('source_se').distinct('source_se').filter(finish_time__gte = notBefore)
    if 'term' in httpRequest.GET and str(httpRequest.GET['term']) != '':
        query = query.filter(source_se__icontains = httpRequest.GET['term'])
    
    ses = []
    for se in query:
        ses.append(se['source_se'])
    return HttpResponse(simplejson.dumps(ses), mimetype='application/json')



def uniqueDestinations(httpRequest):
    notBefore = datetime.datetime.now() - datetime.timedelta(hours = 12)
    
    query = Job.objects.values('dest_se').distinct('dest_se').filter(finish_time__gte = notBefore)
    if 'term' in httpRequest.GET and str(httpRequest.GET['term']) != '':
        query = query.filter(dest_se__icontains = httpRequest.GET['term'])
    
    ses = []
    for se in query:
        ses.append(se['dest_se'])
    return HttpResponse(simplejson.dumps(ses), mimetype='application/json')



def uniqueVos(httpRequest):
    notBefore = datetime.datetime.now() - datetime.timedelta(hours = 12)
    
    query = Job.objects.values('vo_name').distinct('vo_name').filter(finish_time__gte = notBefore)
    if 'term' in httpRequest.GET and str(httpRequest.GET['term']) != '':
        query = query.filter(vo_name__icontains = httpRequest.GET['term'])
    
    vos = []
    for vo in query:
        vos.append(vo['vo_name'])

    return HttpResponse(simplejson.dumps(vos), mimetype='application/json')
