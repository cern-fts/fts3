cmake_minimum_required(VERSION 2.6)

set(CMAKE_BUILD_TYPE Debug)
ADD_DEFINITIONS(-DBOOST_DISABLE_ASSERTS)


add_subdirectory(generic)
if (EPELBUILD STREQUAL "OFF")
add_subdirectory(oracle)
endif ()
add_subdirectory(mysql)
add_subdirectory(python)
add_subdirectory(schema)

