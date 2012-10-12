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


add_library           (fts3db SHARED fts3db.cpp
                                     DbIfceWrapper.cpp
                                     Jobs.cpp
                                     Se.cpp)
target_link_libraries (fts3db ${Boost_LIBRARIES} fts_db_generic fts_common fts_config)
set_target_properties (fts3db PROPERTIES PREFIX "")

install (TARGETS fts3db
         LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
