import json
from datetime import datetime
from decorator import decorator
from django.core.paginator import Paginator, PageNotAnInteger, EmptyPage
from django.db.models import Model
from django.db.models.query import ValuesQuerySet
from django.core import serializers
from django.http import HttpResponse


class ClassEncoder(json.JSONEncoder):

    def __init__(self, *args, **kwargs):
        json.JSONEncoder.__init__(self, *args, **kwargs)
        self.visited = []


    def default(self, obj):
        if obj in self.visited:
            return
        
        if isinstance(obj, datetime):
            return obj.strftime('%Y-%m-%dT%H:%M:%S%z')
        elif not isinstance(obj, dict) and hasattr(obj, '__iter__'):
            self.visited.append(obj)
            return [self.default(o) for o in obj]
        elif isinstance(obj, Model):
            self.visited.append(obj)
            values = {}
            for k, v in obj.__dict__.iteritems():
                if not k.startswith('_'):
                    values[k] = self.default(v)
            return values
        else:
            return obj



@decorator
def jsonify(f, *args, **kwargs):
    d = f(*args, **kwargs)
    j = json.dumps(d, cls = ClassEncoder, indent = 2, sort_keys = False)
    return HttpResponse(j, mimetype='application/json')



def getPage(paginator, request):
    try:
        if 'page' in request.GET: 
            page = paginator.page(request.GET.get('page'))
        else:
            page = paginator.page(1)
    except PageNotAnInteger:
        page = paginator.page(1)
    except EmptyPage:
        page = paginator.page(paginator.num_pages)
    return page



@decorator
def jsonify_paged(f, *args, **kwargs):
    d = f(*args, **kwargs)
    
    if args[0].GET.get('page', 0) != 'all':
        paginator = Paginator(d, 50)
    else:
        paginator = Paginator(d, len(d))
    page = getPage(paginator, args[0])
    
    paged = {
        'count':      paginator.count,
        'endIndex':   page.end_index(),
        'startIndex': page.start_index(),
        'page':       page.number,
        'pageCount':  paginator.num_pages,
        'items':      page.object_list
    }
    
    j = json.dumps(paged, cls = ClassEncoder, indent = 2, sort_keys = False)
    return HttpResponse(j, mimetype='application/json')
