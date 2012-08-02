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

cmake_minimum_required(VERSION 2.6)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

install(FILES         fts3-msg-bulk.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
	RENAME fts3-msg-bulk )

install(FILES         fts3-msg-cron.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
	RENAME fts3-msg-cron )

install(FILES         fts3-server.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
	RENAME fts3-server )

install(FILES         fts3-msg-cron.logrotate
        DESTINATION ${SYSCONF_INSTALL_DIR}/logrotate.d 
	RENAME fts3-msg-cron )

install(FILES         fts3-server.logrotate
        DESTINATION ${SYSCONF_INSTALL_DIR}/logrotate.d 
	RENAME fts3-server )

install(FILES         fts3-msg-bulk.logrotate
        DESTINATION ${SYSCONF_INSTALL_DIR}/logrotate.d
        RENAME fts3-msg-bulk )
