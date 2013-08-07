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

from django import template
import settings

register = template.Library()

def _transferLogEntry(baseUrl, path):
    return """<li><a href="%(base)s%(path)s">%(path)s</a></li>""" % {'base': baseUrl,
                                                                     'path': path}
@register.simple_tag
def urlTransferLog(transfer):
    if transfer.log_file:
        baseUrl = settings.LOG_BASE_URL.replace('%(host)', transfer.transferHost).strip('/')
        block = "<ul>\n" + _transferLogEntry(baseUrl, transfer.log_file)
        if transfer.log_debug:
            block += _transferLogEntry(baseUrl, transfer.log_file + ".debug")
        block += "\n</ul>"
        return block
    else:
        return "None"



@register.simple_tag
def urlServerLog(server):
    baseUrl = settings.LOG_BASE_URL.replace('%(host)', server['hostname']).strip('/')
    return baseUrl + '/var/log/fts3/fts3server.log'



@register.simple_tag
def urlBringonlineLog(server):
    baseUrl = settings.LOG_BASE_URL.replace('%(host)', server['hostname']).strip('/')
    return baseUrl + '/var/log/fts3/fts3bringonline.log'
