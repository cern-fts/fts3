cmake_minimum_required(VERSION 2.6)

include_directories(BEFORE ${OCCI_INCLUDE_DIR})
include_directories(BEFORE ${LOG4CPP_INCLUDE_DIR})
include_directories (BEFORE ../generic)
include_directories (.)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts3_db_oracle_SOURCES OracleAPI.cpp OracleConnection.cpp OracleTypeConversions.cpp)

add_library(fts3_db_oracle SHARED ${fts3_db_oracle_SOURCES})
target_link_libraries(fts3_db_oracle ${OCCI_LIBRARY} )
target_link_libraries(fts3_db_oracle ${CLNTSH_LIBRARY} )
target_link_libraries(fts3_db_oracle ${LOG4CPP_LIBRARIES} )
target_link_libraries(fts3_db_oracle -lpthread )

MESSAGE( STATUS "OCCI_INCLUDE_DIR: " ${OCCI_INCLUDE_DIR} )
MESSAGE( STATUS "OCCI_LIBRARY: " ${OCCI_LIBRARY} )
MESSAGE( STATUS "CLNTSH_LIBRARY: " ${CLNTSH_LIBRARY} )

MESSAGE( STATUS "LOG4CPP_INCLUDE_DIR: " ${LOG4CPP_INCLUDE_DIR} )
MESSAGE( STATUS "LOG4CPP_LIBRARIES: " ${LOG4CPP_LIBRARIES} )
