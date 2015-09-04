cmake_minimum_required(VERSION 2.6)

set(fts_db_profiled Profiled.cpp)

add_library(fts_db_profiled SHARED ${fts_db_profiled})

target_link_libraries (fts_db_profiled fts_profiler)

set_target_properties(fts_db_profiled    PROPERTIES
                                         VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                         SOVERSION ${VERSION_MAJOR})

if (SERVERBUILD)
install(TARGETS         fts_db_profiled
        RUNTIME         DESTINATION ${CMAKE_INSTALL_PREFIX} 
        LIBRARY         DESTINATION ${LIB_INSTALL_DIR}
        NAMELINK_SKIP)
endif ()

