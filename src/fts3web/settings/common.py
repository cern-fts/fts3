# Copyright notice:
# Copyright (C) Members of the EMI Collaboration, 2010.
#
# See www.eu-emi.eu for details on the copyright holders
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import re
import sys
from ConfigParser import RawConfigParser
from StringIO import StringIO

# INI Configuration
FTS3WEB_CONFIG = RawConfigParser()
if 'FTS3WEB_CONFIG' in os.environ:
    FTS3WEB_CONFIG.read(os.environ['FTS3WEB_CONFIG'])
else:
    FTS3WEB_CONFIG.read('/etc/fts3web/fts3web.ini')

# Load /etc/fts3/fts3config if exists
fts3cfg = None
if os.path.exists('/etc/fts3/fts3config'):
    # FTS3 config file does not have a default header, and ConfigParser does not like that
    content = "[fts3]\n" + open('/etc/fts3/fts3config').read()
    fts3cfg = RawConfigParser()
    fts3cfg.readfp(StringIO(content))

# Overload if /etc/fts3/fts3config is present
if not FTS3WEB_CONFIG.has_section('database'):
    print >>sys.stderr, "[database] not present in the fts3web configuration file"
    print >>sys.stderr, "           Using /etc/fts3/fts3config instead"
    if not fts3cfg:
        raise Exception('[database] not present, and could not load /etc/fts3/fts3config')

    dbType = fts3cfg.get('fts3', 'DbType')
    dbUser = fts3cfg.get('fts3', 'DbUserName')
    dbPass = fts3cfg.get('fts3', 'DbPassword')
    dbName = fts3cfg.get('fts3', 'DbConnectString')
    dbHost = ''
    dbPort = ''

    # Need some translation
    if dbType == 'sqlite':
        dbType = 'sqlite3'
    elif dbType == 'mysql':
        match = re.match('([\w\-.]+)(:(\d+))?/(\S+)', dbName)
        if not match:
            raise ValueError('Could not parse %s' % dbName)
        (dbHost, dbPort, dbName) = match.group(1, 3, 4)


    # Copy to configuration
    FTS3WEB_CONFIG.add_section('database')
    FTS3WEB_CONFIG.set('database', 'engine',   dbType)
    FTS3WEB_CONFIG.set('database', 'user',     dbUser)
    FTS3WEB_CONFIG.set('database', 'password', dbPass)
    FTS3WEB_CONFIG.set('database', 'name',     dbName)
    FTS3WEB_CONFIG.set('database', 'host',     dbHost)
    FTS3WEB_CONFIG.set('database', 'port',     dbPort)

    if FTS3WEB_CONFIG.get('database', 'engine') == 'sqlite':
        FTS3WEB_CONFIG.set('database', 'engine', 'sqlite3')

if not FTS3WEB_CONFIG.get('site', 'name'):
    FTS3WEB_CONFIG.set('site', 'name', fts3cfg.get('fts3', 'SiteName'))

###
if 'BASE_URL' in os.environ:
    BASE_URL = os.environ['BASE_URL']
else:
    BASE_URL = ''

DEBUG = FTS3WEB_CONFIG.getboolean('server', 'debug')
TEMPLATE_DEBUG = DEBUG

TIME_ZONE = None
LANGUAGE_CODE = 'en-gb'

# Project root
# Note: dirname is called twice to get the parent of settings
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Set up paths
sys.path.append("%s/apps" % PROJECT_ROOT)
sys.path.append("%s/libs" % PROJECT_ROOT)

# If you set this to False, Django will make some optimizations so as not
# to load the internationalization machinery.
USE_I18N = False

# Absolute path to the directory static files should be collected to.
# Don't put anything in this directory yourself; store your static files
# in apps' "static/" subdirectories and in STATICFILES_DIRS.
# Example: "/home/media/media.lawrence.com/static/"
STATIC_ROOT = ''

# URL prefix for static files.
# Example: "http://media.lawrence.com/static/"
STATIC_URL = '%s/media/' % BASE_URL

# Additional locations of static files
STATICFILES_DIRS = (
    '%s/media' % PROJECT_ROOT,
)

# List of finder classes that know how to find static files in
# various locations.
STATICFILES_FINDERS = (
    'django.contrib.staticfiles.finders.FileSystemFinder',
    'django.contrib.staticfiles.finders.AppDirectoriesFinder',
)

# Make this unique, and don't share it with anybody.
SECRET_KEY = 'sysadmins-to-change-this'

# List of callables that know how to import templates from various sources.
TEMPLATE_LOADERS = (
    'django.template.loaders.filesystem.Loader',
    'django.template.loaders.app_directories.Loader',
)

TEMPLATE_CONTEXT_PROCESSORS = (
    'django.core.context_processors.static',
)

MIDDLEWARE_CLASSES = (
    'django.middleware.common.CommonMiddleware',
    'django.contrib.sessions.middleware.SessionMiddleware',
    'django.middleware.csrf.CsrfViewMiddleware',
    'django.contrib.auth.middleware.AuthenticationMiddleware',
    'django.contrib.messages.middleware.MessageMiddleware',
)

ROOT_URLCONF = 'urls'

TEMPLATE_DIRS = (
)

INSTALLED_APPS = (
    'django.contrib.sessions',
    'django.contrib.messages',
    'django.contrib.staticfiles',
    'ftsmon'
)

# Do not use a DB
SESSION_ENGINE='django.contrib.sessions.backends.cache'

# A sample logging configuration
LOGGING = {
    'version': 1,
# Uncomment this to log the queries
#    'handlers': {
#        'console': {
#            'class': 'logging.StreamHandler',
#            'level': 'DEBUG'
#        }
#    },
#   'loggers': {
#        'django.db': {
#            'handlers': ['console'],
#            'level': 'DEBUG'
#        }
#   }
}
