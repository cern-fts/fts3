cmake_minimum_required(VERSION 2.8)

find_package (GLOBUS)
include_directories (${GLOBUS_INCLUDE_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_proxy_SOURCES CredUtility.cpp DelegCred.cpp TempFile.cpp)

add_library(fts_proxy SHARED ${fts_proxy_SOURCES})
add_dependencies(fts_proxy fts_common fts_db_generic)
target_link_libraries(fts_proxy fts_common fts_db_generic)
target_link_libraries(fts_proxy ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(fts_proxy -lglobus_common -lglobus_gsi_credential)
target_link_libraries(fts_proxy -lvomsapi)

set_target_properties(fts_proxy		        PROPERTIES
                                                LIBRARY_OUTPUT_DIRECTORY
${PROJECT_BINARY_DIR}/src/cred
                                                VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                                SOVERSION ${VERSION_MAJOR}
                                                CLEAN_DIRECT_OUTPUT 1)


install(TARGETS         fts_proxy 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
