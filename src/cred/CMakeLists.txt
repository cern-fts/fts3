cmake_minimum_required(VERSION 2.6)

include_directories (.)
include_directories (../common)
include_directories (../db/generic)
include_directories (/usr/include/globus)
include_directories (/usr/lib64/globus/include)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${fts3_include_dir})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_proxy_SOURCES cred-utility.cpp CredService.cpp DelegCred.cpp utility.cpp)

add_library(fts3_proxy SHARED ${fts3_proxy_SOURCES})
target_link_libraries(fts3_proxy -ldl )
target_link_libraries(fts3_proxy -lpthread )
target_link_libraries(fts3_proxy -lfts3_common )
target_link_libraries(fts3_proxy -lfts3_config -lglobus_common -lglobus_gsi_credential )

install(TARGETS         fts3_proxy 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
