# Copyright notice:
#
# Copyright (C) CERN 2013-2015
#
# Copyright (C) Members of the EMI Collaboration, 2010-2013.
#   See www.eu-emi.eu for details on the copyright holders
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

try:
    from django.conf.urls.defaults import patterns, url
except:
    from django.conf.urls import patterns, url


urlpatterns = patterns('ftsmon.views',
    url(r'^$', 'index.index'),

    url(r'^overview$', 'overview.get_overview'),
    url(r'^overview/atlas$', 'atlas.get_overview'),

    url(r'^jobs/?$',                               'jobs.get_job_list'),
    url(r'^jobs/(?P<job_id>[a-fA-F0-9\-]+)$',       'jobs.get_job_details'),
    url(r'^jobs/(?P<job_id>[a-fA-F0-9\-]+)/files$', 'jobs.get_job_transfers'),

    url(r'^transfers$', 'jobs.get_transfer_list'),

    url(r'^config/audit$',  'config.get_audit'),
    url(r'^config/links$',  'config.get_link_config'),
    url(r'^config/server$', 'config.get_server_config'),
    url(r'^config/debug$',  'config.get_debug_config'),
    url(r'^config/limits$', 'config.get_limit_config'),
    url(r'^config/range',   'config.get_ranges'),
    url(r'^config/gfal2$',  'config.get_gfal2_config'),
    url(r'^config/activities$', 'config.get_activities'),
    url(r'^config/activities/(?P<vo>(.+))$', 'config.get_actives_per_activity'),

    url(r'^stats$',            'statistics.get_overview'),
    url(r'^stats/servers$',    'statistics.get_servers'),
    url(r'^stats/vo$',         'statistics.get_pervo'),
    url(r'^stats/volume$',     'statistics.get_transfer_volume'),

    url(r'^plot/pie',   'plots.draw_pie'),
    url(r'^plot/lines', 'plots.draw_lines'),

    url(r'^optimizer/$',         'optimizer.get_optimizer_pairs'),
    url(r'^optimizer/detailed$', 'optimizer.get_optimizer_details'),
    url(r'^optimizer/streams',   'optimizer.get_optimizer_streams'),

    url(r'^errors/$',     'errors.get_errors'),
    url(r'^errors/list$', 'errors.get_errors_for_pair'),

    url(r'^unique/activities',   'autocomplete.get_unique_activities'),
    url(r'^unique/destinations', 'autocomplete.get_unique_destinations'),
    url(r'^unique/sources',      'autocomplete.get_unique_sources'),
    url(r'^unique/vos',          'autocomplete.get_unique_vos'),
    url(r'^unique/hostnames',    'autocomplete.get_unique_hostnames')
)
