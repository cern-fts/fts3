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

find_package (GLIB2)
include_directories (${GLIB2_INCLUDE_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set(fts_include_dir "${PROJECT_SOURCE_DIR}/src/")
#include_directories (${fts_include_dir})

#set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_db_cleaner_SOURCES clean.cpp)

add_executable(fts_db_cleaner ${fts_db_cleaner_SOURCES})
add_dependencies(fts_db_cleaner fts_config)
add_dependencies(fts_db_cleaner fts_common)
add_dependencies(fts_db_cleaner fts_db_generic)
target_link_libraries(fts_db_cleaner ${CMAKE_DL_LIBS})
target_link_libraries(fts_db_cleaner ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(fts_db_cleaner -lfts_db_generic)
target_link_libraries(fts_db_cleaner -lfts_config)
target_link_libraries(fts_db_cleaner -lfts_common)

install(TARGETS         fts_db_cleaner 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )
