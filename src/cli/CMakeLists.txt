#
# Copyright (c) CERN 2013-2015
#
# Copyright (c) Members of the EMI Collaboration. 2010-2013
#  See  http://www.eu-emi.eu/partners for details on the copyright
#  holders.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required (VERSION 2.8)

# Set up include directories

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Dependencies
find_package(GLOBUS)
include_directories(${GLOBUS_INCLUDE_DIRS})

find_package(Boost COMPONENTS program_options unit_test_framework REQUIRED)

# Require soap headers
include_directories ("${CMAKE_BINARY_DIR}/src/ws-ifce/gsoap")

add_definitions(-DCURL_STATICLIB)

include_directories(${CURL_INCLUDE_DIR})

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif(DOXYGEN_FOUND)


# Set up libs
set (fts_cli_internal_libs
    fts_common
)

set(fts_cli_libs
    ${fts_cli_internal_libs}
    gridsite
    ${CURL_LIBRARY}
    globus_gsi_sysconfig
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(fts_cli_libs ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${fts_cli_libs})
endif ()

# Common CLI library
file(GLOB fts_cli_common_sources "*.cpp" "ui/*.cpp" "rest/*.cpp" "exception/*.cpp" "delegation/*.cpp")

add_library(fts_cli_common SHARED ${fts_cli_common_sources})
target_link_libraries(fts_cli_common ${fts_cli_libs})
set_target_properties(fts_cli_common PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/cli
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
)

# CLI Tools

# fts-transfer-submit
add_executable(fts-transfer-submit
    transfer/fts3-transfer-submit.cpp
    ui/SubmitTransferCli.cpp
)
target_link_libraries(fts-transfer-submit fts_cli_common)

# fts-delegation-init
add_executable(fts-delegation-init
    transfer/fts3-delegation-init.cpp
    ui/DelegationCli.cpp
)
target_link_libraries(fts-delegation-init fts_cli_common)

# fts-transfer-status
add_executable(fts-transfer-status
    transfer/fts3-transfer-status.cpp
    ui/TransferStatusCli.cpp
)
target_link_libraries(fts-transfer-status fts_cli_common)

# fts-transfer-delete
add_executable(fts-transfer-delete
    transfer/fts3-transfer-delete.cpp
    ui/SrcDelCli.cpp
)
target_link_libraries(fts-transfer-delete fts_cli_common)

# fts-transfer-list
add_executable(fts-transfer-list
    transfer/fts3-transfer-list.cpp
    ui/ListTransferCli.cpp
)
target_link_libraries(fts-transfer-list fts_cli_common)

# fts-transfer-cancel
add_executable(fts-transfer-cancel
    transfer/fts3-transfer-cancel.cpp
)
target_link_libraries(fts-transfer-cancel fts_cli_common)

# fts-set-debug command
add_executable(fts-set-debug
    set/fts3-set-debug.cpp
)
target_link_libraries(fts-set-debug fts_cli_common)

# fts-set-blacklist command
add_executable(fts-set-blacklist
    set/fts3-set-blk.cpp
)
target_link_libraries(fts-set-blacklist fts_cli_common)

# fts-set-priority command
add_executable(fts-set-priority
    set/fts3-set-priority.cpp
)
target_link_libraries(fts-set-priority fts_cli_common)


# Man pages
set(MAN_INPUT_DIR "${PROJECT_SOURCE_DIR}/doc/man/cli")

install(FILES "${MAN_INPUT_DIR}/fts-config-del.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-config-set.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-config-get.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-transfer-cancel.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-transfer-list.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-transfer-status.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-transfer-submit.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-set-priority.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-set-debug.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-set-blacklist.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-transfer-delete.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-delegation-init.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)

# Artifacts
install(TARGETS
	fts-transfer-submit
    fts-transfer-status
    fts-transfer-cancel
    fts-transfer-list
	fts-set-debug
	fts-set-blacklist
	fts-set-priority
	fts-transfer-delete
	fts-delegation-init
	RUNTIME DESTINATION ${BIN_INSTALL_DIR}
)

install(TARGETS fts_cli_common
	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
)
