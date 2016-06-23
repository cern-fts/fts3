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

if (MYSQLBUILD STREQUAL "ON")
	install(FILES       mysql/fts-schema-1.0.0.sql
			DESTINATION ${SHARE_INSTALL_PREFIX}/fts-mysql)
    install(FILES       mysql/fts-diff-1.0.1.sql
            DESTINATION ${SHARE_INSTALL_PREFIX}/fts-mysql)
    install(FILES       mysql/fts-diff-2.0.0.sql
            DESTINATION ${SHARE_INSTALL_PREFIX}/fts-mysql)
endif ()

if (SERVERBUILD STREQUAL "ON")
	install(FILES       fts-database-upgrade.py
			DESTINATION ${SHARE_INSTALL_PREFIX}/fts)
endif ()
