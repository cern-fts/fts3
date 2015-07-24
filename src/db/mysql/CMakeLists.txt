cmake_minimum_required(VERSION 2.8)

find_package(MySQL REQUIRED)

include_directories (BEFORE ../generic)
include_directories (BEFORE ../..)
include_directories (BEFORE ../../common)
include_directories (.)
include_directories (${MYSQL_INCLUDE_DIR})
include_directories (/usr/include/openssl)
include_directories (/usr/include/soci)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_db_mysql_SOURCES MySqlAPI.cpp)


add_library(fts_db_mysql SHARED ${fts_db_mysql_SOURCES})

target_link_libraries(fts_db_mysql dl fts_common soci_core soci_mysql ${MYSQL_LIBRARIES})

set_target_properties(fts_db_mysql             PROPERTIES
                                               LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/db/mysql
                                               VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                               SOVERSION ${VERSION_MAJOR}
                                               CLEAN_DIRECT_OUTPUT 1)


install(TARGETS         fts_db_mysql
        RUNTIME         DESTINATION ${CMAKE_INSTALL_PREFIX} 
        LIBRARY         DESTINATION ${LIB_INSTALL_DIR}
        NAMELINK_SKIP)
