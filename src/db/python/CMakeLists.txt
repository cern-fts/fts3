cmake_minimum_required (VERSION 2.6)

find_package (Boost COMPONENTS python REQUIRED)
find_package (PythonLibs REQUIRED)

execute_process (COMMAND python -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)" 
                 OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
                 OUTPUT_STRIP_TRAILING_WHITESPACE)

include_directories (${Boost_INCLUDE_DIRS}
                     ${PYTHON_INCLUDE_PATH}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_SOURCE_DIR}/../../common)
link_directories(${Boost_LIBRARY_DIRS})


add_library           (ftsdb SHARED fts3db.cpp
                                    MonitoringDbWrapper.cpp
                                    types.cpp)
target_link_libraries (ftsdb ${Boost_LIBRARIES} fts_db_generic fts_common fts_config)
set_target_properties (ftsdb PROPERTIES PREFIX "")

install (TARGETS ftsdb
         LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES}/fts)
