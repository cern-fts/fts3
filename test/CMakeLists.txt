cmake_minimum_required (VERSION 2.6)

project (fts3-tests)

if (NOT DEFINED SHARE_INSTALL_PREFIX)
  SET(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/usr/share")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
install (PROGRAMS FTS3-config
         DESTINATION ${SHARE_INSTALL_PREFIX}/fts3/tests)

install (DIRECTORY functional stress
         DESTINATION ${SHARE_INSTALL_PREFIX}/fts3/tests
         USE_SOURCE_PERMISSIONS)
endif ()


