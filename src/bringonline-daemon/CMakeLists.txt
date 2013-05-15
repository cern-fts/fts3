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
include_directories (/usr/include/gfal2)
link_directories (/usr/lib64/gfal2-plugins)


find_package (GLIB2)
include_directories (${GLIB2_INCLUDE_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(fts_bringonline_SOURCES main.cpp)

add_executable(fts_bringonline ${fts_bringonline_SOURCES})
add_dependencies(fts_bringonline fts_config)
add_dependencies(fts_bringonline fts_common)
add_dependencies(fts_bringonline fts_db_generic)
add_dependencies(fts_bringonline fts_proxy)

find_package (Boost COMPONENTS thread system)

target_link_libraries(fts_bringonline ${CMAKE_DL_LIBS})
target_link_libraries(fts_bringonline ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(fts_bringonline -lfts_db_generic)
target_link_libraries(fts_bringonline -lfts_config)
target_link_libraries(fts_bringonline -lfts_common)
target_link_libraries(fts_bringonline -lfts_proxy)
target_link_libraries(fts_bringonline  -luuid -lssl -lgfal_transfer -lgfal2
                                       -lglib-2.0 ${fts_libs}
                                       ${Boost_LIBRARIES})

install(TARGETS         fts_bringonline 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )		
	
install(FILES         fts-bringonline.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
	RENAME fts-bringonline )
