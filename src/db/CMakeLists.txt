cmake_minimum_required(VERSION 2.6)

set(CMAKE_BUILD_TYPE Debug)
ADD_DEFINITIONS(-DBOOST_DISABLE_ASSERTS)


add_subdirectory(generic)
add_subdirectory(oracle)
add_subdirectory(python)
add_subdirectory(schema)
#add_subdirectory(tests)
