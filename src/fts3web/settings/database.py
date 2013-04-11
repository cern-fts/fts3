from common import FTS3WEB_CONFIG

DATABASES = {
    'default': {
        'ENGINE':   'django.db.backends.' + FTS3WEB_CONFIG.get('database', 'engine'),
        'USER':     FTS3WEB_CONFIG.get('database', 'user'),
        'PASSWORD': FTS3WEB_CONFIG.get('database', 'password'),
        'NAME':     FTS3WEB_CONFIG.get('database', 'name'),
        'HOST':     FTS3WEB_CONFIG.get('database', 'host'),
        'PORT':     FTS3WEB_CONFIG.get('database', 'port')
    }
}

if DATABASES['default']['ENGINE'] == 'django.db.backends.oracle':
    DATABASES['default']['OPTIONS'] = {'threaded': True}
