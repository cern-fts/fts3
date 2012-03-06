cmake_minimum_required(VERSION 2.6)

include_directories (.)
include_directories (../../common)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${fts3_include_dir})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_db_generic_SOURCES SingleDbInstance.cpp DynamicLibraryManager.cpp DynamicLibraryManagerException.cpp)

add_library(fts3_db_generic SHARED ${fts3_db_generic_SOURCES})
target_link_libraries(fts3_db_generic -ldl )
target_link_libraries(fts3_db_generic ${LOG4CPP_LIBRARIES} )
target_link_libraries(fts3_db_generic -lpthread )
target_link_libraries(fts3_db_generic -lfts3_common )
target_link_libraries(fts3_db_generic -lfts3_config )

install(TARGETS         fts3_db_generic 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
