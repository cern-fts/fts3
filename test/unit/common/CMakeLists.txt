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

# =========== Uri ================

add_executable(uri_test "Uri.cpp")

target_link_libraries(uri_test
        fts_common
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(uri_test uri_test)

# ======== Thread Pool =============

add_executable(thread_pool_test "ThreadPool.cpp")

target_link_libraries(thread_pool_test
        fts_common
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(thread_pool_test thread_pool_test)

# ======== PID Tools =============

add_executable(pid_tools_test "PidTools.cpp")

target_link_libraries(pid_tools_test
        fts_common
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(pid_tools_test pid_tools_test)

# ======== Panic =============

add_executable(panic_test "panic.cpp")

target_link_libraries(panic_test
        fts_common
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(panic_test panic_test)

# ======== Logger =============

add_executable(logger_test "Logger.cpp")

target_link_libraries(logger_test
        fts_common
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(logger_test logger_test)

# ======== DaemonTools =============

add_executable(daemon_tools_test "DaemonTools.cpp")

target_link_libraries(daemon_tools_test
        fts_common
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(daemon_tools_test daemon_tools_test)

# ======== ConcurrentQueue =============

add_executable(concurrent_queue_test "ConcurrentQueue.cpp")

target_link_libraries(concurrent_queue_test
        fts_common
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(concurrent_queue_test concurrent_queue_test)
