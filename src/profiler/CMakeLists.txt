cmake_minimum_required(VERSION 2.8)

include_directories (BEFORE ../)
include_directories (BEFORE ../common)
include_directories (BEFORE ../db/generic)
include_directories (/usr/include/soci)

add_library (fts_profiler SHARED Profiler.cpp)

target_link_libraries (fts_profiler rt)

set_target_properties(fts_profiler       PROPERTIES
                                         VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                         SOVERSION ${VERSION_MAJOR})

if (SERVERBUILD)
install(TARGETS         fts_profiler
        RUNTIME         DESTINATION ${CMAKE_INSTALL_PREFIX} 
        LIBRARY         DESTINATION ${LIB_INSTALL_DIR}
        NAMELINK_SKIP)
endif ()

