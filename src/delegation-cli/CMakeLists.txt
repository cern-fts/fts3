cmake_minimum_required(VERSION 2.8)

include_directories (.)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/ws-ifce/gsoap)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/ws-ifce/)

find_package (GLIB2)
include_directories (${GLIB2_INCLUDE_DIRS})

find_package (GLOBUS)
include_directories (${GLOBUS_INCLUDE_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${fts_include_dir})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_delegation_api_simple_SOURCES uri.cpp util.cpp cfg.cpp delegation_simple_api.cpp)

add_library(fts_delegation_api_simple SHARED ${fts_delegation_api_simple_SOURCES})
add_dependencies(fts_delegation_api_simple fts_delegation_api_cpp)
target_link_libraries(fts_delegation_api_simple -lgridsite -lglib-2.0 -lgsoap++ -lcgsi_plugin_cpp -lis_ifce )
target_link_libraries(fts_delegation_api_simple fts_delegation_api_cpp )
set_target_properties(fts_delegation_api_simple PROPERTIES
LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/delegation-cli VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} SOVERSION ${VERSION_MAJOR} CLEAN_DIRECT_OUTPUT 1)

install(TARGETS         fts_delegation_api_simple 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
