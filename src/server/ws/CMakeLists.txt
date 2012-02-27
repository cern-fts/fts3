cmake_minimum_required(VERSION 2.6)

project (fts3-transfer)

set(CMAKE_CXX_FLAGS "-g -Wall")

include_directories("${PROJECT_SOURCE_DIR}")

add_executable( fts3-transfer-srv
        srv.cpp
        ftsC.cpp
        ftsFileTransferSoapBindingService.cpp
        FtsServiceTask.cpp
        UuidGenerator.cpp
)

target_link_libraries(fts3-transfer-srv )
target_link_libraries(fts3-transfer-srv gsoap++)
target_link_libraries(fts3-transfer-srv uuid)
target_link_libraries(fts3-transfer-srv pthread)
target_link_libraries(fts3-transfer-srv boost_thread)
target_link_libraries(fts3-transfer-srv fts3_db_generic)



