from django.conf.urls.defaults import patterns, url

urlpatterns = patterns('ftsmon.views',
    url(r'^$', 'jobIndex'),
    url(r'^jobs/?$', 'jobIndex'),
    url(r'^jobs/(?P<jobId>[a-fA-F0-9\-]+)$', 'jobDetails'),
    url(r'^queue$', 'jobQueue'),
    url(r'^stats$', 'statistics'),
)
