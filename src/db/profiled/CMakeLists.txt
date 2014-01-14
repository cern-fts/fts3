cmake_minimum_required(VERSION 2.8)

include_directories (BEFORE ../generic)
include_directories (BEFORE ../../common)
include_directories (BEFORE ../../)

set(fts_db_profiled Profiled.cpp)

add_library(fts_db_profiled SHARED ${fts_db_profiled})

target_link_libraries (fts_db_profiled fts_profiler)

if (MAINBUILD STREQUAL "ON" OR ALLBUILD STREQUAL "ON")
install(TARGETS         fts_db_profiled
        RUNTIME         DESTINATION ${CMAKE_INSTALL_PREFIX} 
        LIBRARY         DESTINATION ${LIB_INSTALL_DIR})
endif ()

