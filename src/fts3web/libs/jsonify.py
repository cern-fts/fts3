import json
import sys
from datetime import datetime, time
from decorator import decorator
from django.db.models import Model
from django.db.models.query import ValuesQuerySet
from django.core import serializers
from django.http import HttpResponse
from util import paged


class ClassEncoder(json.JSONEncoder):

    def __init__(self, *args, **kwargs):
        json.JSONEncoder.__init__(self, *args, **kwargs)
        self.visited = []


    def default(self, obj):
        if obj in self.visited:
            return
        
        if isinstance(obj, datetime):
            return obj.strftime('%Y-%m-%dT%H:%M:%S%z')
        elif isinstance(obj, time):
            return obj.strftime('%H:%M:%S')
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
    j = json.dumps(d, cls = ClassEncoder, indent = 2, sort_keys = True)
    return HttpResponse(j, mimetype='application/json')


@decorator
def jsonify_paged(f, *args, **kwargs):
    d = f(*args, **kwargs)
    j = json.dumps(paged(d, args[0]), cls = ClassEncoder, indent = 2, sort_keys = False)
    return HttpResponse(j, mimetype='application/json')
