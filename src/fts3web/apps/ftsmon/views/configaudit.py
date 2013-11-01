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
from ftsweb.models import ConfigAudit
from ftsweb.models import ProfilingSnapshot, ProfilingInfo
from jsonify import jsonify, jsonify_paged

@jsonify_paged
def configurationAudit(httpRequest):
    ca = ConfigAudit.objects
    
    if httpRequest.GET.get('action', None):
        ca = ca.filter(action = httpRequest.GET['action'])
    if httpRequest.GET.get('user', None):
        ca = ca.filter(dn = httpRequest.GET['user'])
    if httpRequest.GET.get('contains', None):
        ca = ca.filter(config__icontains = httpRequest.GET['contains'])
    
    return ca.order_by('-datetime')
