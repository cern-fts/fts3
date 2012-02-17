cmake_minimum_required(VERSION 2.6)

include_directories (.)
include_directories (../../common)
include_directories(BEFORE ${LOG4CPP_INCLUDE_DIR})



set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_db_generic_SOURCES SingleDbInstance.cpp DynamicLibraryManager.cpp DynamicLibraryManagerException.cpp)

add_library(fts3_db_generic SHARED ${fts3_db_generic_SOURCES})
target_link_libraries(fts3_db_generic -ldl )
target_link_libraries(fts3_db_generic ${LOG4CPP_LIBRARIES} )
target_link_libraries(fts3_db_generic -lpthread )
target_link_libraries(fts3_db_generic -lfts3_common )

