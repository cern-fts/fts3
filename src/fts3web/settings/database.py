
# Oracle example
DATABASES = {
    'default': {
        'ENGINE':   'django.db.backends.oracle',
        'USER':     'FTS3',
        'PASSWORD': 'FTS3',
        'NAME':     'XE',
        'HOST':     'localhost',
        'PORT':     '1521',
        'OPTIONS': {
            'threaded': True,
        }
    }
}

# MySQL example
#DATABASES = {
#    'default': {
#        'ENGINE':   'django.db.backends.mysql',
#        'USER':     'fts3',
#        'PASSWORD': 'fts3',
#        'NAME':     'fts3',
#        'HOST':     'arioch.cern.ch',
#    }
#}
