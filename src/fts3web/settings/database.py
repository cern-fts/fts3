import os
from common import FTS3WEB_CONFIG
from ConfigParser import RawConfigParser

DATABASES = {
    'default': {
        'ENGINE':   'django.db.backends.' + FTS3WEB_CONFIG.get('database', 'engine'),
        'USER':     FTS3WEB_CONFIG.get('database', 'user'),
        'PASSWORD': FTS3WEB_CONFIG.get('database', 'password'),
        'NAME':     FTS3WEB_CONFIG.get('database', 'name'),
        'HOST':     FTS3WEB_CONFIG.get('database', 'host'),
        'PORT':     FTS3WEB_CONFIG.get('database', 'port'),
        'OPTIONS':  {}
    }
}

if DATABASES['default']['ENGINE'] == 'django.db.backends.oracle':
    DATABASES['default']['OPTIONS']['threaded'] = True

if DATABASES['default']['ENGINE'] == 'django.db.backends.mysql':
    DATABASES['default']['OPTIONS']['init_command'] = "SET SESSION TRANSACTION ISOLATION LEVEL READ UNCOMMITTED"
