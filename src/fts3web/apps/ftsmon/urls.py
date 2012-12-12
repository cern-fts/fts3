from django.conf.urls.defaults import patterns, url

urlpatterns = patterns('ftsmon.views',
    url(r'^$', 'jobIndex'),
    url(r'^jobs/?$', 'jobIndex'),
    url(r'^jobs/(?P<jobId>[a-fA-F0-9\-]+)$', 'jobDetails'),
    url(r'^queue$', 'jobQueue'),
    url(r'^stats$', 'statistics'),
    url(r'^configuration$', 'configurationAudit'),
    
    url(r'^json/uniqueSources', 'uniqueSources'),
    url(r'^json/uniqueDestinations', 'uniqueDestinations'),
    url(r'^json/uniqueVos', 'uniqueVos'),
    
    url(r'^plot/pie', 'pie'),
)
