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
import os
from django.db import connection
from django.db.models import Q
from ftsweb.models import ConfigAudit
from ftsweb.models import LinkConfig, ShareConfig
from ftsweb.models import DebugConfig, Optimize
from authn import require_certificate
from jsonify import jsonify, jsonify_paged
from util import get_order_by, ordered_field


@require_certificate
@jsonify_paged
def get_audit(http_request):
    ca = ConfigAudit.objects

    if http_request.GET.get('action', None):
        ca = ca.filter(action=http_request.GET['action'])
    if http_request.GET.get('user', None):
        ca = ca.filter(dn=http_request.GET['user'])
    if http_request.GET.get('contains', None):
        ca = ca.filter(config__icontains=http_request.GET['contains'])

    return ca.order_by('-datetime')


@require_certificate
@jsonify
def get_server_config(http_request):
    config = dict()
    cursor = connection.cursor()
    cursor.execute("SELECT retry, max_time_queue, global_timeout, sec_per_mb, vo_name FROM t_server_config")
    server_config = cursor.fetchall()
    config['per_vo'] = list()
    for entry in server_config:
        c = dict(
            retry=entry[0],
            max_time_queue=entry[1],
            global_timeout=entry[2],
            sec_per_mb=entry[3],
            vo_name=entry[4]
        )
        config['per_vo'].append(c)

    cursor.execute("SELECT mode_opt FROM t_optimize_mode")
    modes = cursor.fetchall()
    if len(modes) > 0:
        config['optimizer_mode'] = modes[0][0]
    return config


# Wrap a list of link config, and push the
# vo shares on demand (lazy!)
class AppendShares:

    def __init__(self, result_set):
        self.rs = result_set

    def __len__(self):
        return len(self.rs)

    def __getitem__(self, i):
        for link in self.rs[i]:
            shares = ShareConfig.objects.filter(source=link.source, destination=link.destination).all()
            link.shares = {}
            for share in shares:
                link.shares[share.vo] = share.active
            yield link


@require_certificate
@jsonify_paged
def get_link_config(http_request):
    links = LinkConfig.objects

    if http_request.GET.get('source_se'):
        links = links.filter(source=http_request.GET['source_se'])
    if http_request.GET.get('dest_se'):
        links = links.filter(destination=http_request.GET['dest_se'])

    return AppendShares(links.all())


@require_certificate
@jsonify_paged
def get_debug_config(http_request):
    return DebugConfig.objects.order_by('source_se', 'dest_se')


@require_certificate
@jsonify_paged
def get_limit_config(http_request):
    max_cfg = Optimize.objects.filter(Q(active__isnull=False) | Q(bandwidth__isnull=False))

    (order_by, order_desc) = get_order_by(http_request)
    if order_by == 'bandwidth':
        max_cfg = max_cfg.order_by(ordered_field('bandwidth', order_desc))
    elif order_by == 'active':
        max_cfg = max_cfg.order_by(ordered_field('active', order_desc))
    elif order_by == 'source_se':
        max_cfg = max_cfg.order_by(ordered_field('source_se', order_desc))
    elif order_by == 'dest_se':
        max_cfg = max_cfg.order_by(ordered_field('dest_se', order_desc))
    else:
        max_cfg = max_cfg.order_by('-active')

    return max_cfg


@require_certificate
@jsonify
def get_gfal2_config(http_request):
    try:
        config_files = os.listdir('/etc/gfal2.d')
    except:
        config_files = list()
    config_files = filter(lambda c: c.endswith('.conf'), config_files)

    config = dict()
    for cfg in config_files:
        cfg_path = os.path.join('/etc/gfal2.d', cfg)
        config[cfg_path] = open(cfg_path).read()

    return config
