cmake_minimum_required(VERSION 2.8)


set(fts_myosg_updater_SOURCES updater.cpp)

find_package (Boost COMPONENTS system REQUIRED)

add_executable(fts_myosg_updater ${fts_myosg_updater_SOURCES})
add_dependencies(fts_myosg_updater fts_config)
add_dependencies(fts_myosg_updater fts_common)
target_link_libraries(fts_myosg_updater -lcurl)
target_link_libraries(fts_myosg_updater fts_config)
if(FTS3_COMPILE_WITH_UNITTEST_NEW)
	target_link_libraries(fts_myosg_updater -lboost_unit_test_framework)
endif()
target_link_libraries(fts_myosg_updater fts_common)
target_link_libraries(fts_myosg_updater ${Boost_SYSTEM_LIBRARIES})

install(TARGETS         fts_myosg_updater 
	RUNTIME             DESTINATION ${SBIN_INSTALL_DIR} )

install(FILES         fts-myosg-updater
        DESTINATION ${SYSCONF_INSTALL_DIR}/cron.daily
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ)			
	
install(FILES         fts-myosg-updater.initd
        DESTINATION ${SYSCONF_INSTALL_DIR}/rc.d/init.d 
	PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ WORLD_READ
	RENAME fts-myosg-updater )
	
install(FILES         myosg.xml
        DESTINATION /var/lib/fts3 
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
