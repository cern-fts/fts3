cmake_minimum_required(VERSION 2.8)

find_package (Boost COMPONENTS system)

find_package (GLIB2)
include_directories (${GLIB2_INCLUDE_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set(fts_include_dir "${PROJECT_SOURCE_DIR}/src/")
#include_directories (${fts_include_dir})

#set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_db_cleaner_SOURCES clean.cpp)

add_executable(fts_db_cleaner ${fts_db_cleaner_SOURCES})
add_dependencies(fts_db_cleaner fts_config)
add_dependencies(fts_db_cleaner fts_common)
add_dependencies(fts_db_cleaner fts_db_generic)
target_link_libraries(fts_db_cleaner ${CMAKE_DL_LIBS})
target_link_libraries(fts_db_cleaner ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(fts_db_cleaner fts_db_generic)
target_link_libraries(fts_db_cleaner fts_config)
if(FTS3_COMPILE_WITH_UNITTEST_NEW)
	target_link_libraries(fts_db_cleaner -lboost_unit_test_framework)
endif()
target_link_libraries(fts_db_cleaner fts_common)
target_link_libraries(fts_db_cleaner ${Boost_LIBRARIES})

install(TARGETS         fts_db_cleaner 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )
