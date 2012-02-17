cmake_minimum_required(VERSION 2.6)

include_directories (.)
include_directories(BEFORE ${LOG4CPP_INCLUDE_DIR})



set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_common_SOURCES Logger.cpp ReadConfigFile.cpp)

add_library(fts3_common SHARED ${fts3_common_SOURCES})
target_link_libraries(fts3_common ${LOG4CPP_LIBRARIES} )
target_link_libraries(fts3_common -lpthread )


