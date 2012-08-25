cmake_minimum_required(VERSION 2.6)

include_directories (.)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/ws-ifce/gsoap)
include_directories (/usr/include/glib-2.0)
include_directories (/usr/lib64/glib-2.0/include)
include_directories (/usr/include/globus)
include_directories (/usr/lib64/globus/include)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${fts3_include_dir})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_delegation_api_simple_SOURCES uri.cpp util.cpp cfg.cpp delegation_simple_api.cpp)

add_library(fts3_delegation_api_simple SHARED ${fts3_delegation_api_simple_SOURCES})
target_link_libraries(fts3_delegation_api_simple -lgridsite -lglib-2.0 -lgsoap++ -lcgsi_plugin_cpp )
target_link_libraries(fts3_delegation_api_simple -lfts3_delegation_api_cpp )
#target_link_libraries(fts3_delegation_api_simple -lfts3_common )
#target_link_libraries(fts3_delegation_api_simple -lfts3_config )

install(TARGETS         fts3_delegation_api_simple 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
