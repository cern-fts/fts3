cmake_minimum_required(VERSION 2.6)

include_directories (BEFORE ../)

add_library (fts_profiler SHARED Profiler.cpp)

target_link_libraries (fts_profiler rt)
