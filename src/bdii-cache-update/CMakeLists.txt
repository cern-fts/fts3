cmake_minimum_required(VERSION 2.8)

set(fts_include_dir 
    "${PROJECT_SOURCE_DIR}/src/"
    "${CMAKE_BINARY_DIR}/src/"
)

include_directories (${fts_include_dir})
include_directories (.)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/infosys/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/infosys/)

#find_package (GLIB2)
#include_directories (${GLIB2_INCLUDE_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(fts_bdii_cache_updater_SOURCES updater.cpp)

find_package (Boost COMPONENTS system REQUIRED)

add_executable(fts_bdii_cache_updater ${fts_bdii_cache_updater_SOURCES})
add_dependencies(fts_bdii_cache_updater fts_common)
add_dependencies(fts_bdii_cache_updater fts_config)
add_dependencies(fts_bdii_cache_updater fts_infosys)
if(FTS3_COMPILE_WITH_UNITTEST_NEW)
	target_link_libraries(fts_bdii_cache_updater -lboost_unit_test_framework)
endif()
target_link_libraries(fts_bdii_cache_updater fts_common)
target_link_libraries(fts_bdii_cache_updater fts_config)
target_link_libraries(fts_bdii_cache_updater fts_infosys)
target_link_libraries(fts_bdii_cache_updater ${Boost_SYSTEM_LIBRARIES})
target_link_libraries(fts_bdii_cache_updater -lpugixml)

install(TARGETS         fts_bdii_cache_updater 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )

install(FILES         fts-bdii-cache-updater
        DESTINATION ${SYSCONF_INSTALL_DIR}/cron.daily
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ)			

if (SYSTEMD_INSTALL_DIR)
    install(FILES         fts-bdii-cache-updater.service
            DESTINATION ${SYSTEMD_INSTALL_DIR})
else ()
    install(FILES         fts-bdii-cache-updater.initd
            DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
	    PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
	    RENAME fts-bdii-cache-updater )
endif ()
	
install(FILES         bdii_cache.xml
        DESTINATION /var/lib/fts3 
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
	

