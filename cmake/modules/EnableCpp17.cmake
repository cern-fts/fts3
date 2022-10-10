include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++17 HAVE_FLAG_STD_CXX17)

if (NOT HAVE_FLAG_STD_CXX17)
    message(FATAL_ERROR "Compiler with C++17 features (-std=c++17) is required!")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
