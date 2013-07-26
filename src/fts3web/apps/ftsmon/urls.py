# Copyright notice:
# Copyright © Members of the EMI Collaboration, 2010.
#
# See www.eu-emi.eu for details on the copyright holders
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
