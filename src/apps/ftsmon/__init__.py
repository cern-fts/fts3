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

import logging

from django.db import connection
from ftsweb.models import DnField


# Hook a signal handler to enable/disable DN display
def dn_disable_callback(sender, **kwargs):
    log = logging.getLogger(__name__)
    log.debug('Connection initiated')
    cursor = connection.cursor()
    cursor.execute('SELECT show_user_dn FROM t_server_config')
    row = cursor.fetchone()
    hide = (row is not None and row[0] is not None and row[0].lower() == 'off')
    log.debug('Hide users\' dn: %s' % hide)
    DnField.hide_dn = hide

from django.db.backends.signals import connection_created
connection_created.connect(dn_disable_callback)
