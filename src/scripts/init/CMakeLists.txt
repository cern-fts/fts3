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

# Systemd services
if (SYSTEMD_INSTALL_DIR)
    install(FILES fts-server.service
                  fts-msg-bulk.service
                  fts-records-cleaner.service
                  fts-qos.service
                  fts-optimizer.service
                  fts-token.service
                  ftstokenrefresherd.service
                  ftstokenhousekeeperd.service
        DESTINATION ${SYSTEMD_INSTALL_DIR}
    )
    if (FTS4SCHEDULERBUILD)
        install(FILES fts-scheduler.service
            DESTINATION ${SYSTEMD_INSTALL_DIR}
        )
    endif ()
# Init scripts
else ()
    install(FILES fts-server.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d
        PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
        RENAME fts-server
    )

    install(FILES fts-msg-bulk.initd
       DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d
       PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
       RENAME fts-msg-bulk
    )

    install(FILES fts-records-cleaner.initd
       DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d
       PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
       RENAME fts-records-cleaner
    )
    
    install(FILES fts-qos.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d
        PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
        RENAME fts-qos
    )
endif ()
