from django.conf.urls.defaults import patterns, url

urlpatterns = patterns('ftsmon.views',
    url(r'^$', 'jobIndex'),
    url(r'^jobs/?$', 'jobIndex'),
    url(r'^jobs/(?P<jobId>[a-fA-F0-9\-]+)$', 'jobDetails'),
    url(r'^queue$', 'queue'),
    url(r'^staging', 'staging'),
    url(r'^stats$', 'statistics'),
    url(r'^configuration$', 'configurationAudit'),
    
    url(r'^json/uniqueSources/$', 'uniqueSources', {'archive': None}),
    url(r'^json/uniqueDestinations/$', 'uniqueDestinations', {'archive': None}),
    url(r'^json/uniqueVos/$', 'uniqueVos', {'archive': None}),
    url(r'^json/uniqueSources/(?P<archive>[a-z]*?)$', 'uniqueSources'),
    url(r'^json/uniqueDestinations/(?P<archive>[a-z]*)$', 'uniqueDestinations'),
    url(r'^json/uniqueVos/(?P<archive>[a-z]*)$', 'uniqueVos'),
    
    url(r'^plot/pie', 'pie'),
    
    url(r'^optimizer/$', 'optimizer'),
    url(r'^optimizer/detailed$', 'optimizerDetailed'),
    
    url(r'^errors/$', 'showErrors'),
    url(r'^errors/list$', 'transfersWithError'),
    
    url(r'^archive/$', 'archiveJobIndex')
)
