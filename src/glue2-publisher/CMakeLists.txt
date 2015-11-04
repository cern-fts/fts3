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

cmake_minimum_required(VERSION 2.8)

find_package (Boost COMPONENTS system filesystem REQUIRED)
find_package (GLIB2)
include_directories (${GLIB2_INCLUDE_DIRS})


set(fts_info_publisher_SOURCES publisher.cpp)
add_executable(fts_info_publisher ${fts_info_publisher_SOURCES})
target_link_libraries(fts_info_publisher
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    fts_db_generic
    fts_config
    fts_common
    ${Boost_LIBRARIES}
)
if(FTS3_COMPILE_WITH_UNITTEST_NEW)
    target_link_libraries(fts_info_publisher -lboost_unit_test_framework)
endif()

# Artifacts
install(TARGETS fts_info_publisher
    RUNTIME DESTINATION ${SBIN_INSTALL_DIR}
)

install(FILES fts-info-publisher
    DESTINATION ${SYSCONF_INSTALL_DIR}/cron.hourly
    PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
)

if (SYSTEMD_INSTALL_DIR)
    install(FILES fts-info-publisher.service
        DESTINATION ${SYSTEMD_INSTALL_DIR}
    )
else ()
    install(FILES fts-info-publisher.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d
        PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
        RENAME fts-info-publisher
    )
endif ()

