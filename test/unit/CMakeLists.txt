#
# Copyright (c) CERN 2024
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

add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(cred)
add_subdirectory(db)
add_subdirectory(msg-bus)
add_subdirectory(server)
add_subdirectory(url-copy)

# Sources
file(GLOB fts_tests_SOURCES "*/*.cpp")

add_executable(fts-unit-tests ${fts_tests_SOURCES})

target_link_libraries(fts-unit-tests
        fts_proxy
        fts_db_generic
        fts_common
        fts_msg_bus
        fts_server_lib
        fts_msg_ifce
        fts_url_copy_lib
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

install(TARGETS fts-unit-tests
        RUNTIME DESTINATION ${BIN_INSTALL_DIR})

add_test(fts-unit-tests fts-unit-tests)
