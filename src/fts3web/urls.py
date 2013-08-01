from django.conf.urls.defaults import patterns, include, url
from django.views.generic.base import RedirectView

urlpatterns = patterns('',
    url(r'^ftsmon/', include('ftsmon.urls')),
    url(r'^$', RedirectView.as_view(url = 'ftsmon/'))    
)
