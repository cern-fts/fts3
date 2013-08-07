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

register = template.Library()

@register.filter
def partition(dictlist, key):
    array = []
    for item in dictlist:
        array.append(item[str(key)])
    return array


@register.simple_tag(takes_context = True)
def with_query(context, arg_name, arg_value):
	request = context['request']
	pre_existing = {}
	pre_existing.update(request.GET.iteritems())
	
	pre_existing[arg_name] = arg_value
	
	return '&'.join(map(lambda (x,y): "%s=%s" % (x, y), pre_existing.iteritems()))
