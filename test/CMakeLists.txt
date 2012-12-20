cmake_minimum_required (VERSION 2.6)

project (fts3-tests)


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
install (PROGRAMS FTS3-config
                  Macros
         DESTINATION ${DOC_INSTALL_DIR}/tests)

install (DIRECTORY functional stress
         DESTINATION ${DOC_INSTALL_DIR}/tests)
endif ()


