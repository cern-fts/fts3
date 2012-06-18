cmake_minimum_required(VERSION 2.6)


# GSOAP file generation - general part
find_package(gSOAP)

include_directories (.)
include_directories (../../common)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_include_dir "${PROJECT_SOURCE_DIR}/src/")
include_directories (${fts3_include_dir})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
