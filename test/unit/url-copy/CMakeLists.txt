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

find_package(GFAL2)
find_package(GLIB2)
include_directories(
        ${GLIB2_INCLUDE_DIRS}
        ${GFAL2_INCLUDE_DIRS}
)

# ================ Auto Interrupt Test =====================

add_executable(auto_interrupt_thread_test "AutoInterruptThread.cpp")

target_link_libraries(auto_interrupt_thread_test
        fts_url_copy_lib
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(auto_interrupt_thread_test auto_interrupt_thread_test)

# ================ Auto Interrupt Test =====================

add_executable(dest_file_report_test "DestFileReport.cpp")

target_link_libraries(dest_file_report_test
        fts_url_copy_lib
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(dest_file_report_test dest_file_report_test)

# ================ UrlCopyProcess Test =====================

add_executable(url_copy_process_test "UrlCopyProcess.cpp")

target_link_libraries(url_copy_process_test
        fts_url_copy_lib
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(url_copy_process_test url_copy_process_test)
