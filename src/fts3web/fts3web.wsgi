#!/usr/bin/env python
import os
import sys

path = os.path.dirname(__file__)
if path not in sys.path:
  sys.path.append(path)

os.environ['DJANGO_SETTINGS_MODULE'] = 'settings'

import django.core.handlers.wsgi
_application = django.core.handlers.wsgi.WSGIHandler()

def application(environ, start_response):
  os.environ['BASE_URL'] = environ['SCRIPT_NAME']
  return _application(environ, start_response)
  
