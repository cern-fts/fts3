cmake_minimum_required (VERSION 2.6)

project (fts3-tests)

install (PROGRAMS FTS3-config
                  Macros
         DESTINATION share/fts3/tests)

install (DIRECTORY functional stress
         DESTINATION share/fts3/tests)

