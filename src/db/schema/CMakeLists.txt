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

if (ORACLEBUILD STREQUAL "ON")
install(FILES         oracle-drop.sql
        DESTINATION ${SHARE_INSTALL_PREFIX}/fts-oracle )

install(FILES         oracle-schema.sql
        DESTINATION ${SHARE_INSTALL_PREFIX}/fts-oracle )
endif ()

if (MYSQLBUILD STREQUAL "ON")

set(APPLICATION_NAME fts-mysql)

install(FILES         mysql-schema.sql
        DESTINATION ${SHARE_INSTALL_PREFIX}/fts-mysql )

install(FILES         mysql-drop.sql
        DESTINATION ${SHARE_INSTALL_PREFIX}/fts-mysql )

install(FILES         mysql-truncate.sql
        DESTINATION ${SHARE_INSTALL_PREFIX}/fts-mysql )
endif ()

if (ALLBUILD STREQUAL "ON")
install(FILES         oracle-drop.sql
        DESTINATION ${SHARE_INSTALL_PREFIX}/fts-oracle )

install(FILES         oracle-schema.sql
        DESTINATION ${SHARE_INSTALL_PREFIX}/fts-oracle )
	
	install(FILES         mysql-schema.sql
        DESTINATION ${SHARE_INSTALL_PREFIX}/fts-mysql )

install(FILES         mysql-drop.sql
        DESTINATION ${SHARE_INSTALL_PREFIX}/fts-mysql )

install(FILES         mysql-truncate.sql
        DESTINATION ${SHARE_INSTALL_PREFIX}/fts-mysql )
endif ()
