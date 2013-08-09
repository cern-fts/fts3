cmake_minimum_required(VERSION 2.6)

include_directories (BEFORE ../)
include_directories (BEFORE ../common)
include_directories (BEFORE ../db/generic)

add_library (fts_profiler SHARED Profiler.cpp)

target_link_libraries (fts_profiler rt)

if (MAINBUILD STREQUAL "ON" OR ALLBUILD STREQUAL "ON")
install(TARGETS         fts_profiler
        RUNTIME         DESTINATION ${CMAKE_INSTALL_PREFIX} 
        LIBRARY         DESTINATION ${LIB_INSTALL_DIR}
        NAMELINK_SKIP)
endif ()

