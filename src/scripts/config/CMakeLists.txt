#
# Copyright (c) CERN 2013-2015
#
# Copyright (c) Members of the EMI Collaboration. 2010-2013
#  See  http://www.eu-emi.eu/partners for details on the copyright
#  holders.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set (CMAKE_INCLUDE_CURRENT_DIR ON)

install(FILES fts-server.sysconfig
    DESTINATION ${SYSCONF_INSTALL_DIR}/sysconfig
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    RENAME fts-server
)

install(FILES fts-qos.sysconfig
    DESTINATION ${SYSCONF_INSTALL_DIR}/sysconfig
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    RENAME fts-qos
)

install(FILES fts-optimizer.sysconfig
    DESTINATION ${SYSCONF_INSTALL_DIR}/sysconfig
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    RENAME fts-optimizer
)

install(FILES fts-server.logrotate
    DESTINATION ${SYSCONF_INSTALL_DIR}/logrotate.d
    RENAME fts-server
)
