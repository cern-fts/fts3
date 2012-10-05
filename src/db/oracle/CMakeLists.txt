cmake_minimum_required(VERSION 2.6)

include_directories(BEFORE ${OCCI_INCLUDE_DIR})
include_directories (BEFORE ../generic)
include_directories (BEFORE ../../common)
include_directories (.)



set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_db_oracle_SOURCES OracleAPI.cpp OracleConnection.cpp OracleTypeConversions.cpp)

add_library(fts_db_oracle SHARED ${fts_db_oracle_SOURCES})
target_link_libraries(fts_db_oracle ${OCCI_LIBRARY} )
target_link_libraries(fts_db_oracle ${CLNTSH_LIBRARY} )
target_link_libraries(fts_db_oracle -lpthread )

MESSAGE( STATUS "OCCI_INCLUDE_DIR: " ${OCCI_INCLUDE_DIR} )
MESSAGE( STATUS "OCCI_LIBRARY: " ${OCCI_LIBRARY} )
MESSAGE( STATUS "CLNTSH_LIBRARY: " ${CLNTSH_LIBRARY} )

install(TARGETS         fts_db_oracle 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )

