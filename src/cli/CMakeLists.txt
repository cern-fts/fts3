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

FIND_PACKAGE(PythonLibs REQUIRED)
include_directories (${fts_include_dir} ${PYTHON_INCLUDE_PATH})

find_package (Boost COMPONENTS python REQUIRED)
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
    cgsi_plugin_cpp
    boost_program_options-mt
    boost_regex-mt
    fts_delegation_api_simple
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(fts_cli_libs boost_unit_test_framework ${fts_cli_libs})
endif ()

# ------------------------------------------------------------------------
# Common CLI library

set(fts_cli_common_sources
	ui/CliBase.cpp
	ui/TransferCliBase.cpp
	ui/DebugSetCli.cpp
	ui/DnCli.cpp
	ui/GetCfgCli.cpp
	ui/JobIdCli.cpp
	ui/ListTransferCli.cpp
	ui/SetCfgCli.cpp
	ui/SrcDestCli.cpp
	ui/SubmitTransferCli.cpp
	ui/TransferStatusCli.cpp
	ui/VoNameCli.cpp
	ui/BlacklistCli.cpp
	ui/PriorityCli.cpp
    GSoapContextAdapter.cpp
    ProxyCertificateDelegator.cpp
    MsgPrinter.cpp
)

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
# fts-transfer-status

add_executable(fts-transfer-status
    transfer/fts3-transfer-status.cpp
    ui/TransferStatusCli.cpp 
)
add_dependencies(fts-transfer-status fts_cli_common)
target_link_libraries (fts-transfer-status fts_cli_common)

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
# fts-get-log command

#add_executable(fts-get-log 
#    set/fts3-get-log.cpp
#)
#target_link_libraries (fts-get-log fts_cli_common)

# ------------------------------------------------------------------------

# ------------------------------------------------------------------------
# python bindings
add_library(ftspython SHARED python/PythonBindings.cpp python/PythonApi.cpp python/Job.cpp python/FileTransfer.cpp)
target_link_libraries(ftspython fts_cli_common python2.6 boost_python)
#add_dependencies(ftspython fts_cli_common)

# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
# man pages

set(MAN_INPUT_DIR "${PROJECT_SOURCE_DIR}/doc/man/cli")
set(MAN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/man1")
set(MAN_INSTALL_DIR "/usr/share/man/man1")

file(MAKE_DIRECTORY ${MAN_OUTPUT_DIR})

find_package(GZip)

# find all manpages filenames
file(GLOB man_pages RELATIVE ${MAN_INPUT_DIR} ${MAN_INPUT_DIR}/fts-*)

# loop over all manpages
foreach(man_page ${man_pages})
	# gzip the manpage
	add_custom_command(
		TARGET ${man_page}
		POST_BUILD
		COMMAND ${GZIP} -c ${MAN_INPUT_DIR}/${man_page} > ${MAN_OUTPUT_DIR}/${man_page}.1.gz
	)
	#install it
	install(
		FILES ${MAN_OUTPUT_DIR}/${man_page}.1.gz
		DESTINATION ${MAN_INSTALL_DIR}
	)
	
endforeach()
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
	RUNTIME             DESTINATION ${BIN_INSTALL_DIR} )

install(TARGETS         fts_cli_common 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
	
install (TARGETS ftspython
         LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES}/fts)	
