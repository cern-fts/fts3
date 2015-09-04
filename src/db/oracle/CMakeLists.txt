cmake_minimum_required(VERSION 2.8)

find_package(OCCI)

include_directories(BEFORE ${OCCI_INCLUDE_DIR})
include_directories (/usr/include/soci)

set(fts_db_oracle_SOURCES OracleAPI.cpp)

add_library(fts_db_oracle SHARED ${fts_db_oracle_SOURCES})
add_dependencies(fts_db_oracle fts_common)
target_link_libraries(fts_db_oracle soci_core soci_oracle)
target_link_libraries(fts_db_oracle fts_common)
set_target_properties(fts_db_oracle             PROPERTIES
                                                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/db/oracle
                                                VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                                SOVERSION ${VERSION_MAJOR}
                                                CLEAN_DIRECT_OUTPUT 1)


MESSAGE( STATUS "OCCI_INCLUDE_DIR: " ${OCCI_INCLUDE_DIR} )
MESSAGE( STATUS "OCCI_LIBRARY: " ${OCCI_LIBRARY} )
MESSAGE( STATUS "CLNTSH_LIBRARY: " ${CLNTSH_LIBRARY} )

install(TARGETS         fts_db_oracle 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} 
    NAMELINK_SKIP)
