cmake_minimum_required(VERSION 2.6)

set(fts_include_dir 
    "${PROJECT_SOURCE_DIR}/src/"
    "${CMAKE_BINARY_DIR}/src/"
)

include_directories (${fts_include_dir})
include_directories (.)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/cred/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/db/generic/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/server/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/server/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/db/generic/)
include_directories (/usr/include/glib-2.0)
include_directories (/usr/lib64/glib-2.0/include)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_info_publisher_SOURCES publisher.cpp)

add_executable(fts_info_publisher ${fts_info_publisher_SOURCES})
add_dependencies(fts_info_publisher fts_config)
add_dependencies(fts_info_publisher fts_common)
add_dependencies(fts_info_publisher fts_db_generic)
target_link_libraries(fts_info_publisher -ldl)
target_link_libraries(fts_info_publisher -lpthread)
target_link_libraries(fts_info_publisher -lfts_db_generic)
target_link_libraries(fts_info_publisher -lfts_config)
target_link_libraries(fts_info_publisher -lfts_common)

install(TARGETS         fts_info_publisher 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )

install(FILES         fts-info-publisher
        DESTINATION ${SYSCONF_INSTALL_DIR}/cron.hourly
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ)			
	
install(FILES         fts-info-publisher.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
	RENAME fts-info-publisher )
