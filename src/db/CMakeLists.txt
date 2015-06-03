cmake_minimum_required(VERSION 2.8)

add_definitions(-DBOOST_DISABLE_ASSERTS)

add_subdirectory(generic)
add_subdirectory(profiled)
add_subdirectory(schema)

if (ORACLEBUILD)
add_subdirectory(oracle)
endif ()

if (MYSQLBUILD)
add_subdirectory(mysql)
endif ()

if (SERVERBUILD STREQUAL "ON")
add_subdirectory(python)
endif ()
