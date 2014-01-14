cmake_minimum_required(VERSION 2.8)

ADD_DEFINITIONS(-DBOOST_DISABLE_ASSERTS)

add_subdirectory(generic)
add_subdirectory(profiled)

if (ORACLEBUILD STREQUAL "ON")
add_subdirectory(oracle)
add_subdirectory(schema)
endif ()

if (MYSQLBUILD STREQUAL "ON")
add_subdirectory(mysql)
add_subdirectory(schema)
endif ()

if (ALLBUILD STREQUAL "ON")
add_subdirectory(oracle)
add_subdirectory(mysql)
add_subdirectory(python)
add_subdirectory(schema)
endif ()


if (MAINBUILD STREQUAL "ON")
add_subdirectory(python)
endif ()
