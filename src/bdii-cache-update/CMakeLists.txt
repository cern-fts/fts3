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

set(fts_bdii_cache_updater_SOURCES updater.cpp)

# Extra boost package
find_package(Boost COMPONENTS system REQUIRED)

add_executable(fts_bdii_cache_updater ${fts_bdii_cache_updater_SOURCES})
target_link_libraries(fts_bdii_cache_updater
    fts_common
    fts_config
    fts_infosys
    ${Boost_SYSTEM_LIBRARIES}
    pugixml
)

# Artifacts
install(TARGETS fts_bdii_cache_updater 
	RUNTIME DESTINATION ${SBIN_INSTALL_DIR}
)

install(FILES fts-bdii-cache-updater
    DESTINATION ${SYSCONF_INSTALL_DIR}/cron.daily
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
)			

if (SYSTEMD_INSTALL_DIR)
    install(FILES fts-bdii-cache-updater.service
        DESTINATION ${SYSTEMD_INSTALL_DIR}
    )
else ()
    install(FILES fts-bdii-cache-updater.initd
       DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
       PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
       RENAME fts-bdii-cache-updater
    )
endif ()

install(FILES bdii_cache.xml
    DESTINATION ${DATADIR_INSTALL_PREFIX}
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

