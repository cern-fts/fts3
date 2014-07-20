cmake_minimum_required(VERSION 2.8)

include_directories (.)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/db/generic/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/db/generic/)

find_package (GLOBUS)
include_directories (${GLOBUS_INCLUDE_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${fts_include_dir})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_proxy_SOURCES cred-utility.cpp CredService.cpp DelegCred.cpp utility.cpp)

add_library(fts_proxy SHARED ${fts_proxy_SOURCES})
add_dependencies(fts_proxy fts_common fts_db_generic)
target_link_libraries(fts_proxy fts_common fts_db_generic)
target_link_libraries(fts_proxy ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(fts_proxy -lglobus_common -lglobus_gsi_credential -lgridsite)
set_target_properties(fts_proxy		        PROPERTIES
                                                LIBRARY_OUTPUT_DIRECTORY
${PROJECT_BINARY_DIR}/src/cred
                                                VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                                SOVERSION ${VERSION_MAJOR}
                                                CLEAN_DIRECT_OUTPUT 1)


install(TARGETS         fts_proxy 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
