cmake_minimum_required(VERSION 2.6)

include_directories (.)
include_directories (../generic)
include_directories (../../common)
include_directories (../../config)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${fts3_include_dir})

add_executable(testsoci testsoci.cpp)
target_link_libraries (testsoci fts3_db_generic)
target_link_libraries (testsoci -lboost_thread-mt)
target_link_libraries (testsoci -lfts3_config)
