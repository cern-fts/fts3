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

find_package(GFAL2)
find_package(GLIB2)
find_package(ZMQ)
find_package(JSONCPP)
find_package(Cryptopp)

include_directories(
    ${GLIB2_INCLUDE_DIRS}
    ${GFAL2_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${CRYPTOPP_INCLUDE_DIRS}
)

add_library(fts_url_copy_lib SHARED
    LogHelper.cpp
    heuristics.cpp
    LegacyReporter.cpp
    Transfer.cpp
    UrlCopyOpts.cpp
    UrlCopyProcess.cpp
    Callbacks.cpp
)
target_link_libraries(fts_url_copy_lib
    ${GLIB2_LIBRARIES}
    ${GFAL2_LIBRARIES}
    ${ZMQ_LIBRARIES}
    ${CRYPTOPP_LIBRARIES}
    JSONCPP::JSONCPP
    fts_common
    fts_msg_bus
    fts_msg_ifce
)
set_target_properties(fts_url_copy_lib
    PROPERTIES OUTPUT_NAME fts_url_copy
)

add_executable(fts_url_copy
    main.cpp
)

target_link_libraries(fts_url_copy
    fts_url_copy_lib
)

# Install artifacts
install(TARGETS fts_url_copy_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(TARGETS fts_url_copy
    RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
)
