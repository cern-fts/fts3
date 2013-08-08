cmake_minimum_required(VERSION 2.6)

include_directories (BEFORE ../generic)
include_directories (BEFORE ../../common)

set(fts_db_profiled Profiled.cpp)

add_library(fts_db_profiled SHARED ${fts_db_profiled})
