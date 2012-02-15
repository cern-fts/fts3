cmake_minimum_required(VERSION 2.6)

set(CMAKE_BUILD_TYPE Debug)
ADD_DEFINITIONS(-DBOOST_DISABLE_ASSERTS)

add_subdirectory(db)
add_subdirectory(server)
add_subdirectory(common)
add_subdirectory(ws-ifce)
add_subdirectory(cli)


