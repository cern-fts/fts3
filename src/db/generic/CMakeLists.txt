cmake_minimum_required(VERSION 2.6)

set(fts_include_dir 
    "${PROJECT_SOURCE_DIR}/src/"
    "${CMAKE_BINARY_DIR}/src/"
)

include_directories (${fts_include_dir})
include_directories (.)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set(fts_include_dir "${PROJECT_SOURCE_DIR}/src/")
#include_directories (${fts_include_dir})

#set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_db_generic_SOURCES SingleDbInstance.cpp DynamicLibraryManager.cpp DynamicLibraryManagerException.cpp)

add_library(fts_db_generic SHARED ${fts_db_generic_SOURCES})
add_dependencies(fts_db_generic fts_config)
target_link_libraries(fts_db_generic -ldl)
target_link_libraries(fts_db_generic -lpthread)
#target_link_libraries(fts_db_generic -lfts_config)
set_target_properties(fts_db_generic            PROPERTIES
                                                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/db/generic
                                                VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                                SOVERSION ${VERSION_MAJOR}
                                                CLEAN_DIRECT_OUTPUT 1)

if (MAINBUILD STREQUAL "ON" OR ALLBUILD STREQUAL "ON")
install(TARGETS         fts_db_generic 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
endif ()
