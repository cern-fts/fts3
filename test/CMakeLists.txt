cmake_minimum_required (VERSION 2.6)

project (fts3-tests)


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
install (PROGRAMS FTS3-config
                  Macros
         DESTINATION ${SHARE_INSTALL_PREFIX}/tests)

install (DIRECTORY functional stress
         DESTINATION ${SHARE_INSTALL_PREFIX}/tests)
endif ()


