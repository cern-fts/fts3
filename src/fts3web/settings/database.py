
# Oracle example
DATABASES = {
    'default': {
        'ENGINE':   'django.db.backends.oracle',
        'USER':     'system',
        'PASSWORD': 'dssi73',
        'NAME':     'XE',
        'HOST':     'fts2source.cern.ch',
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
