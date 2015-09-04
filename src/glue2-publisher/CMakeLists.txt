cmake_minimum_required(VERSION 2.8)


find_package (GLIB2)
include_directories (${GLIB2_INCLUDE_DIRS})

set(fts_info_publisher_SOURCES publisher.cpp)

find_package (Boost COMPONENTS system REQUIRED)

add_executable(fts_info_publisher ${fts_info_publisher_SOURCES})
add_dependencies(fts_info_publisher fts_config)
add_dependencies(fts_info_publisher fts_common)
add_dependencies(fts_info_publisher fts_db_generic)
target_link_libraries(fts_info_publisher ${CMAKE_DL_LIBS})
target_link_libraries(fts_info_publisher ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(fts_info_publisher fts_db_generic)
target_link_libraries(fts_info_publisher fts_config)
if(FTS3_COMPILE_WITH_UNITTEST_NEW)
	target_link_libraries(fts_info_publisher -lboost_unit_test_framework)
endif()
target_link_libraries(fts_info_publisher fts_common)
target_link_libraries(fts_info_publisher ${Boost_SYSTEM_LIBRARIES})

install(TARGETS         fts_info_publisher 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )

install(FILES         fts-info-publisher
        DESTINATION ${SYSCONF_INSTALL_DIR}/cron.hourly
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ)			
	
install(FILES         fts-info-publisher.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
	RENAME fts-info-publisher )
