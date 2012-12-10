from django.conf.urls.defaults import patterns, include, url
from django.views.generic.simple import redirect_to

urlpatterns = patterns('',
    url(r'^ftsmon/', include('ftsmon.urls')),
    url(r'^$', redirect_to, {'url': 'ftsmon/'})    
)
