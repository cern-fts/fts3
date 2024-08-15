include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++20 HAVE_FLAG_STD_CXX20)

if (NOT HAVE_FLAG_STD_CXX20)
    message(FATAL_ERROR "Compiler with C++20 features (-std=c++20) is required!")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
