# Copyright @ Members of the EMI Collaboration, 2010.
# See www.eu-emi.eu for details on the copyright holders.
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
cmake_minimum_required (VERSION 2.6)

# ------------------------------------------------------------------------
# Set up include directories

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(fts_include_dir
    "${PROJECT_SOURCE_DIR}/src/"
    "${CMAKE_BINARY_DIR}/src/"
)

find_package (GLOBUS)
include_directories (${GLOBUS_INCLUDE_DIRS})

FIND_PACKAGE(PythonLibs REQUIRED)
include_directories (${fts_include_dir} ${PYTHON_INCLUDE_PATH})
include_directories (BEFORE ${PROJECT_SOURCE_DIR}/src/common/)

find_package (Boost COMPONENTS python program_options unit_test_framework REQUIRED)
execute_process (COMMAND python -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)" 
                 OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
                 OUTPUT_STRIP_TRAILING_WHITESPACE)

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


# ------------------------------------------------------------------------
# Set up libs

set (fts_cli_internal_libs
    fts_ws_ifce_client
    fts_common
)

set(fts_cli_libs
    ${fts_cli_internal_libs}
    gridsite
    curl
    cgsi_plugin_cpp
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    fts_delegation_api_simple
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(fts_cli_libs ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${fts_cli_libs})
endif ()

# ------------------------------------------------------------------------
# Common CLI library

file(GLOB fts_cli_common_sources "*.cpp" "ui/*.cpp" "rest/*.cpp" "exception/*.cpp" "delegation/*.cpp")
file(GLOB fts_cli_python_sources "python/*.cpp")

add_library(fts_cli_common SHARED ${fts_cli_common_sources})
target_link_libraries(fts_cli_common ${fts_cli_libs})
add_dependencies(fts_cli_common ${fts_cli_libs})
set_target_properties(fts_cli_common            PROPERTIES
                                                LIBRARY_OUTPUT_DIRECTORY
${PROJECT_BINARY_DIR}/src/cli
                                                VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                                SOVERSION ${VERSION_MAJOR}
                                                CLEAN_DIRECT_OUTPUT 1)

#set_target_properties(ftspython            PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/cli VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} SOVERSION ${VERSION_MAJOR} CLEAN_DIRECT_OUTPUT 1)
# ------------------------------------------------------------------------
# fts-transfer-submit
add_executable(fts-transfer-submit
    transfer/fts3-transfer-submit.cpp
    ui/SubmitTransferCli.cpp
)
add_dependencies(fts-transfer-submit fts_cli_common)
target_link_libraries (fts-transfer-submit fts_cli_common)

# ------------------------------------------------------------------------
# fts-delegation-init
add_executable(fts-delegation-init
    transfer/fts3-delegation-init.cpp
    ui/DelegationCli.cpp
)
add_dependencies(fts-delegation-init fts_cli_common)
target_link_libraries (fts-delegation-init fts_cli_common)

# ------------------------------------------------------------------------
# fts-transfer-snapshot
add_executable(fts-transfer-snapshot
    transfer/fts3-transfer-snapshot.cpp
    ui/SnapshotCli.cpp
)
add_dependencies(fts-transfer-snapshot fts_cli_common)
target_link_libraries (fts-transfer-snapshot fts_cli_common)

# ------------------------------------------------------------------------
# fts-transfer-status

add_executable(fts-transfer-status
    transfer/fts3-transfer-status.cpp
    ui/TransferStatusCli.cpp 
)
add_dependencies(fts-transfer-status fts_cli_common)
target_link_libraries (fts-transfer-status fts_cli_common)

# ------------------------------------------------------------------------
# fts-transfer-delete

add_executable(fts-transfer-delete
    transfer/fts3-transfer-delete.cpp
    ui/SrcDelCli.cpp 
)
add_dependencies(fts-transfer-delete fts_cli_common)
target_link_libraries (fts-transfer-delete fts_cli_common)

# ------------------------------------------------------------------------
# fts-transfer-list

add_executable(fts-transfer-list
    transfer/fts3-transfer-list.cpp
    ui/ListTransferCli.cpp 
)
add_dependencies(fts-transfer-list fts_cli_common)
target_link_libraries (fts-transfer-list fts_cli_common)

# ------------------------------------------------------------------------
# fts-transfer-cancel

add_executable(fts-transfer-cancel
    transfer/fts3-transfer-cancel.cpp
)
add_dependencies(fts-transfer-cancel fts_cli_common)
target_link_libraries (fts-transfer-cancel fts_cli_common)

# ------------------------------------------------------------------------
# FTS3 config transfer - common structures

# ------------------------------------------------------------------------
# fts-config-set command

add_executable(fts-config-set 
    config/fts3-config-set.cpp
)
target_link_libraries (fts-config-set fts_cli_common)

# ------------------------------------------------------------------------
# fts-config-del command

add_executable(fts-config-del 
    config/fts3-config-del.cpp
)
target_link_libraries (fts-config-del fts_cli_common)

# ------------------------------------------------------------------------
# fts-config-get command

add_executable(fts-config-get 
    config/fts3-config-get.cpp
)
target_link_libraries (fts-config-get fts_cli_common)

# ------------------------------------------------------------------------
# fts-set-debug command

add_executable(fts-set-debug 
    set/fts3-set-debug.cpp
)
target_link_libraries (fts-set-debug fts_cli_common)

# ------------------------------------------------------------------------
# fts-set-blacklist command

add_executable(fts-set-blacklist 
    set/fts3-set-blk.cpp
)
target_link_libraries (fts-set-blacklist fts_cli_common)

add_executable(fts-set-priority 
    set/fts3-set-priority.cpp
)
target_link_libraries (fts-set-priority fts_cli_common)

# ------------------------------------------------------------------------

# ------------------------------------------------------------------------
# python bindings
find_package(PythonEasy REQUIRED)

add_library(ftspython SHARED ${fts_cli_python_sources})
target_link_libraries(ftspython fts_cli_common ${Boost_PYTHON_LIBRARIES} ${PYTHON_LIBRARIES})
#add_dependencies(ftspython fts_cli_common)

# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
# man pages

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
install(FILES "${MAN_INPUT_DIR}/fts-transfer-snapshot.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-transfer-delete.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)
install(FILES "${MAN_INPUT_DIR}/fts-delegation-init.1" DESTINATION ${MAN_INSTALL_DIR}/man1/)

# ------------------------------------------------------------------------


install(TARGETS
	fts-transfer-submit
        fts-transfer-status
        fts-transfer-cancel
        fts-transfer-list
	fts-config-get
	fts-config-set
	fts-config-del
	fts-set-debug
	fts-set-blacklist
	fts-set-priority
	fts-transfer-snapshot
	fts-transfer-delete
	fts-delegation-init
	RUNTIME             DESTINATION ${BIN_INSTALL_DIR} )

install(TARGETS         fts_cli_common 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
	
install (TARGETS ftspython
         LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES}/fts)
install (FILES       ${CMAKE_CURRENT_SOURCE_DIR}/python/__init__.py
         DESTINATION ${PYTHON_SITE_PACKAGES}/fts)

