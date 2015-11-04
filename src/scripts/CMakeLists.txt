# Copyright @ Members of the EMI Collaboration, 2010.
# See www.eu-emi.eu for details on the copyright holders.
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

cmake_minimum_required(VERSION 2.8)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

# Sytemd services
if (SYSTEMD_INSTALL_DIR)
    install(FILES         fts-server.service
            DESTINATION ${SYSTEMD_INSTALL_DIR})

    install(FILES         fts-bringonline.service
            DESTINATION ${SYSTEMD_INSTALL_DIR})

    install(FILES         fts-msg-bulk.service
            DESTINATION ${SYSTEMD_INSTALL_DIR})
       
    install(FILES         fts-records-cleaner.service
            DESTINATION ${SYSTEMD_INSTALL_DIR})

# Init scripts
else ()
    install(FILES         fts-server.initd
            DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
       PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
       RENAME fts-server )

    install(FILES         fts-bringonline.initd
            DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d
        PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
        RENAME fts-bringonline )

    install(FILES         fts-msg-bulk.initd
            DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
       PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
       RENAME fts-msg-bulk )
       
    install(FILES         fts-records-cleaner.initd
            DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
       PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
       RENAME fts-records-cleaner )	
endif ()
	
install(FILES         fts-records-cleaner
        DESTINATION ${SYSCONF_INSTALL_DIR}/cron.daily 
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ)
	
install(FILES         fts-server.sysconfig
        DESTINATION ${SYSCONF_INSTALL_DIR}/sysconfig 
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    RENAME fts-server )

install(FILES         fts-server.logrotate
        DESTINATION ${SYSCONF_INSTALL_DIR}/logrotate.d 
	RENAME fts-server )

install(FILES         fts-bringonline.sysconfig
        DESTINATION ${SYSCONF_INSTALL_DIR}/sysconfig 
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    RENAME fts-bringonline )

