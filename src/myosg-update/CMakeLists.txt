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

find_package (Boost COMPONENTS system REQUIRED)

set(fts_myosg_updater_SOURCES updater.cpp)
add_executable(fts_myosg_updater ${fts_myosg_updater_SOURCES})
target_link_libraries(fts_myosg_updater
    curl
    fts_config
    fts_common
    ${Boost_SYSTEM_LIBRARIES}
)
if(FTS3_COMPILE_WITH_UNITTEST_NEW)
    target_link_libraries(fts_myosg_updater -lboost_unit_test_framework)
endif()


# Artifacts
install(TARGETS fts_myosg_updater
    RUNTIME DESTINATION ${SBIN_INSTALL_DIR}
)

install(FILES fts-myosg-updater
    DESTINATION ${SYSCONF_INSTALL_DIR}/cron.daily
    PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
)

if (SYSTEMD_INSTALL_DIR)
    install(FILES fts-myosg-updater.service
        DESTINATION ${SYSTEMD_INSTALL_DIR}
    )
else ()
    install(FILES fts-myosg-updater.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d
        PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
        RENAME fts-myosg-updater
    )
endif ()

install(FILES myosg.xml
    DESTINATION /var/lib/fts3
    PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

