cmake_minimum_required(VERSION 2.6)

include_directories (.)
include_directories (../generic)
include_directories (../../common)


add_executable(testsoci testsoci.cpp)
target_link_libraries (testsoci fts3_db_generic)
