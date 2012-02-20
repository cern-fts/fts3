cmake_minimum_required (VERSION 2.6)
#project (fts3-transfer)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(CMAKE_CXX_FLAGS "-g -Wall")

include_directories("${PROJECT_SOURCE_DIR}")

find_package(GLIB2 REQUIRED)
include_directories(${GLIB2_INCLUDE_DIR})
include_directories(${GLIB2_CFG_INCLUDE_DIR})

#include_directories("/home/simonm/workspace_fts/stage/usr/include")

link_directories("/usr/lib64")
link_directories("/opt/glite/lib64")

add_executable(fts3-transfer-submit fts3-transfer-submit.cpp ftsC.cpp ftsFileTransferSoapBindingProxy.cpp CliBase.cpp SubmitTransferCli.cpp SrvManager.cpp)

target_link_libraries (fts3-transfer-submit gsoap++)
target_link_libraries (fts3-transfer-submit cgsi_plugin_cpp)
target_link_libraries (fts3-transfer-submit gridsite_globus)
target_link_libraries (fts3-transfer-submit boost_program_options)
target_link_libraries (fts3-transfer-submit glite_data_util)

add_executable(fts3-transfer-status fts3-transfer-status.cpp ftsC.cpp ftsFileTransferSoapBindingProxy.cpp CliBase.cpp TransferStatusCli.cpp SrvManager.cpp)

target_link_libraries (fts3-transfer-status gsoap++)
target_link_libraries (fts3-transfer-status cgsi_plugin_cpp)
target_link_libraries (fts3-transfer-status gridsite_globus)
target_link_libraries (fts3-transfer-status boost_program_options)
target_link_libraries (fts3-transfer-status glite_data_util)

