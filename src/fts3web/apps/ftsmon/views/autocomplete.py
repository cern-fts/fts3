import datetime
import simplejson
from django.db.models import Q
from django.http import HttpResponse
from ftsweb.models import Job, JobArchive



def uniqueSources(httpRequest, archive):
    if archive:
        model = JobArchive
    else:
        model = Job
        
    query = model.objects
    if 'term' in httpRequest.GET and str(httpRequest.GET['term']) != '':
        query = query.filter(source_se__icontains = httpRequest.GET['term'])
    query = query.distinct('source_se')
    
    ses = []
    for se in query:
        ses.append(se['source_se'])
    return HttpResponse(simplejson.dumps(ses), mimetype='application/json')



def uniqueDestinations(httpRequest, archive):
    if archive:
        model = JobArchive
    else:
        model = Job
    
    query = model.objects.values('dest_se').distinct('dest_se')
    if 'term' in httpRequest.GET and str(httpRequest.GET['term']) != '':
        query = query.filter(dest_se__icontains = httpRequest.GET['term'])
    
    ses = []
    for se in query:
        ses.append(se['dest_se'])
    return HttpResponse(simplejson.dumps(ses), mimetype='application/json')



def uniqueVos(httpRequest, archive):
    if archive:
        model = JobArchive
    else:
        model = Job
    
    query = model.objects.values('vo_name').distinct('vo_name')
    if 'term' in httpRequest.GET and str(httpRequest.GET['term']) != '':
        query = query.filter(vo_name__icontains = httpRequest.GET['term'])
    
    vos = []
    for vo in query:
        vos.append(vo['vo_name'])

    return HttpResponse(simplejson.dumps(vos), mimetype='application/json')
