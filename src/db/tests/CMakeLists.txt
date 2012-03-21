cmake_minimum_required(VERSION 2.6)

include_directories (.)
include_directories (../generic)
include_directories (../../common)
include_directories (../../config)
include_directories (../../server)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${fts3_include_dir})

add_executable(testsoci testsoci.cpp)
target_link_libraries (testsoci fts3_db_generic)
target_link_libraries (testsoci -lboost_thread-mt)
target_link_libraries (testsoci -lfts3_config)
SET(fts3_libs "-lfts3_config -lfts3_common -lfts3_server_lib")
target_link_libraries(testsoci  -lboost_program_options -lgfal_transfer -lgfal2 -lglib-2.0 ${fts3_libs})
