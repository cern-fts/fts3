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


link_directories (/usr/lib64/gfal2-plugins)

include_directories (/usr/include/gfal2)

find_package (GLIB2)
include_directories (${GLIB2_INCLUDE_DIRS})

find_package (Boost COMPONENTS thread)

set(fts_url_copy_SOURCES args.cpp
                         main.cpp
                         file_management.cpp
                         heuristics.cpp
                         logger.cpp
                         reporter.cpp
                         ${PROJECT_SOURCE_DIR}/src/common/UserProxyEnv.cpp
                         ${PROJECT_SOURCE_DIR}/src/common/name_to_uid.cpp
                         ${PROJECT_SOURCE_DIR}/src/common/producer.cpp
                         ${PROJECT_SOURCE_DIR}/src/common/consumer.cpp
                         ${PROJECT_SOURCE_DIR}/src/common/uuid_generator.cpp)
add_executable (fts_url_copy ${fts_url_copy_SOURCES})
add_dependencies(fts_url_copy fts_msg_ifce)
if(FTS3_COMPILE_WITH_UNITTEST_NEW)
	target_link_libraries(fts_url_copy -lboost_unit_test_framework)
endif()
target_link_libraries(fts_url_copy  -luuid -lssl -lcrypto -lgfal_transfer -lgfal2 -lglib-2.0 fts_msg_ifce fts_proxy ${Boost_LIBRARIES})


install(TARGETS         fts_url_copy 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )
