from common import BASE_URL, FTS3WEB_CONFIG
import os
import urlparse

def _urlize(path):
    url = urlparse.urlparse(path)
    if url.scheme:
        return path
    else:
        return path % {'base': BASE_URL} 

SITE_NAME       = FTS3WEB_CONFIG.get('site', 'name')
SITE_LOGO       = _urlize(FTS3WEB_CONFIG.get('site', 'logo'))
SITE_LOGO_SMALL = _urlize(FTS3WEB_CONFIG.get('site', 'logo_small'))

ADMINS = (
    (FTS3WEB_CONFIG.get('site', 'admin_name'), FTS3WEB_CONFIG.get('site', 'admin_mail'))
)

MANAGERS = ADMINS

if FTS3WEB_CONFIG.get('logs', 'port'):
    LOG_BASE_URL =  "%s://%%(host):%d/%s" % (FTS3WEB_CONFIG.get('logs', 'scheme'),
                                             FTS3WEB_CONFIG.getint('logs', 'port'),
                                             FTS3WEB_CONFIG.get('logs', 'base'))
else:
    LOG_BASE_URL =  "%s://%%(host)/%s" % (FTS3WEB_CONFIG.get('logs', 'scheme'),
                                          FTS3WEB_CONFIG.get('logs', 'base'))
