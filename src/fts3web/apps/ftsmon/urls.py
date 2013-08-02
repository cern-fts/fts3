from django.conf.urls.defaults import patterns, url

urlpatterns = patterns('ftsmon.views',
    url(r'^$', 'jobs.jobIndex'),
    url(r'^jobs/?$', 'jobs.jobIndex'),
    url(r'^jobs/(?P<jobId>[a-fA-F0-9\-]+)$', 'jobs.jobDetails'),
    url(r'^queue$', 'queue.queue'),
    url(r'^staging', 'jobs.staging'),
    url(r'^configuration$', 'statistics.configurationAudit'),
    
    url(r'^stats$', 'statistics.overview'),
    url(r'^stats/servers$', 'statistics.servers'),
    url(r'^stats/pairs$', 'statistics.pairs'),
    url(r'^stats/vo$', 'statistics.pervo'),
    
    url(r'^json/uniqueSources/$', 'autocomplete.uniqueSources', {'archive': None}),
    url(r'^json/uniqueDestinations/$', 'autocomplete.uniqueDestinations', {'archive': None}),
    url(r'^json/uniqueVos/$', 'autocomplete.uniqueVos', {'archive': None}),
    url(r'^json/uniqueSources/(?P<archive>[a-z]*?)$', 'autocomplete.uniqueSources'),
    url(r'^json/uniqueDestinations/(?P<archive>[a-z]*)$', 'autocomplete.uniqueDestinations'),
    url(r'^json/uniqueVos/(?P<archive>[a-z]*)$', 'autocomplete.uniqueVos'),
    
    url(r'^plot/pie', 'plots.pie'),
    
    url(r'^optimizer/$', 'optimizer.optimizer'),
    url(r'^optimizer/detailed$', 'optimizer.optimizerDetailed'),
    
    url(r'^errors/$', 'errors.showErrors'),
    url(r'^errors/list$', 'errors.transfersWithError'),
    
    url(r'^archive/$', 'jobs.archiveJobIndex')
)
