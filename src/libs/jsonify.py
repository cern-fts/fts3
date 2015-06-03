import json
from datetime import datetime, time, timedelta
from decimal import Decimal
from decorator import decorator
from django.db.models import Model
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
            return list(obj)
        elif isinstance(obj, Model):
            self.visited.append(obj)
            values = {}
            for k, v in obj.__dict__.iteritems():
                if not k.startswith('_'):
                    values[k] = v
            return values
        elif isinstance(obj, Decimal):
            return float(obj)
        elif isinstance(obj, timedelta):
            return obj.seconds + obj.days * 24 * 3600
        else:
            return json.JSONEncoder.default(self, obj)


def as_json(d):
    j = json.dumps(d, cls=ClassEncoder, indent=2, sort_keys=True)
    return HttpResponse(j, mimetype='application/json')

@decorator
def jsonify(f, *args, **kwargs):
    d = f(*args, **kwargs)
    j = json.dumps(d, cls=ClassEncoder, indent=2, sort_keys=True)
    return HttpResponse(j, mimetype='application/json')


@decorator
def jsonify_paged(f, *args, **kwargs):
    d = f(*args, **kwargs)
    j = json.dumps(paged(d, args[0]), cls=ClassEncoder, indent=2, sort_keys=False)
    return HttpResponse(j, mimetype='application/json')
