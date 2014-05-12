# Copyright notice:
# Copyright (C) Members of the EMI Collaboration, 2010.
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
    url(r'^$', 'index.index'),
    
    url(r'^overview', 'overview.overview'),
    url(r'^jobs/?$', 'jobs.jobIndex'),
    url(r'^jobs/(?P<jobId>[a-fA-F0-9\-]+)$', 'jobs.jobDetails'),
    url(r'^jobs/(?P<jobId>[a-fA-F0-9\-]+)/files$', 'jobs.jobFiles'),
    url(r'^transfers$', 'jobs.transferList'),
    
    url(r'^config/audit$', 'config.audit'),
    url(r'^config/links$', 'config.links'),
    url(r'^config/server$', 'config.server'),
    url(r'^config/debug$', 'config.debug'),
    url(r'^config/limits', 'config.limits'),
    
    url(r'^stats$', 'statistics.overview'),
    url(r'^stats/servers$', 'statistics.servers'),
    url(r'^stats/vo$', 'statistics.pervo'),
    url(r'^stats/volume$', 'statistics.transferVolume'),
    url(r'^stats/profiling$', 'statistics.profiling'),
    url(r'^stats/slowqueries', 'statistics.slowQueries'),
    
    url(r'^plot/pie', 'plots.pie'),
    url(r'^plot/lines', 'plots.lines'),
    
    url(r'^optimizer/$', 'optimizer.optimizer'),
    url(r'^optimizer/detailed$', 'optimizer.optimizerDetailed'),
    
    url(r'^errors/$', 'errors.showErrors'),
    url(r'^errors/list$', 'errors.errorsForPair'),
    
    url(r'^unique/activities', 'autocomplete.activities'),
    url(r'^unique/destinations', 'autocomplete.destinations'),
    url(r'^unique/sources', 'autocomplete.sources'),
    url(r'^unique/vos', 'autocomplete.vos'),
    url(r'^unique/hostnames', 'autocomplete.hostnames')
)
