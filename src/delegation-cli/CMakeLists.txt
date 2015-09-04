cmake_minimum_required(VERSION 2.8)

find_package (GLIB2)
include_directories (${GLIB2_INCLUDE_DIRS})

find_package (GLOBUS)
include_directories (${GLOBUS_INCLUDE_DIRS})

# Require soap headers
include_directories ("${CMAKE_BINARY_DIR}/src/ws-ifce/gsoap")

set(fts_delegation_api_simple_SOURCES util.cpp cfg.cpp delegation_simple_api.cpp)

add_library(fts_delegation_api_simple SHARED ${fts_delegation_api_simple_SOURCES})
add_dependencies(fts_delegation_api_simple fts_delegation_api_cpp)
target_link_libraries(fts_delegation_api_simple -lgridsite -lglib-2.0 -lgsoap++ -lcgsi_plugin_cpp -lis_ifce )
target_link_libraries(fts_delegation_api_simple fts_delegation_api_cpp )
set_target_properties(fts_delegation_api_simple PROPERTIES
LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/delegation-cli VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} SOVERSION ${VERSION_MAJOR} CLEAN_DIRECT_OUTPUT 1)

install(TARGETS         fts_delegation_api_simple 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
