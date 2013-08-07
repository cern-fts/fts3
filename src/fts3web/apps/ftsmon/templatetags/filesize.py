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
import datetime
import types

register = template.Library()

@register.simple_tag
def filesize(size):
    if size is None:
        return 'Unknown'
    fsize = float(size)
    
    for suffix in ['bytes', 'KiB', 'MiB', 'GiB', 'TiB']:
        if fsize < 1024:
            return "%.2f %s" % (fsize, suffix)
        fsize = fsize / 1024.0
    
    return "%.2f PiB" % fsize
