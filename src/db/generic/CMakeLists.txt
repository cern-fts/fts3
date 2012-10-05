cmake_minimum_required(VERSION 2.6)

include_directories (.)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/config/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/common/)
include_directories (BEFORE ${PROJECT_BINARY_DIR}/src/config/)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${fts_include_dir})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_db_generic_SOURCES SingleDbInstance.cpp DynamicLibraryManager.cpp DynamicLibraryManagerException.cpp)

add_library(fts_db_generic SHARED ${fts_db_generic_SOURCES})
target_link_libraries(fts_db_generic -ldl)
target_link_libraries(fts_db_generic -lpthread)
#target_link_libraries(fts_db_generic -lfts_common )
target_link_libraries(fts_db_generic -lfts_config)

install(TARGETS         fts_db_generic 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
