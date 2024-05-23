/*
 * Copyright (c) CERN 2024
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <fstream>

#include "common/Logger.h"

struct LoggerTestHelper {
    mutable int counter;

    LoggerTestHelper() : counter(0) {}
};


std::ostream &operator<<(std::ostream &os, const LoggerTestHelper &helper) {
    ++helper.counter;
    os << "TEST";
    return os;
}


TEST(LogTest, LogLevel) {
    fts3::common::Logger &logger = fts3::common::theLogger();
    logger.setLogLevel(fts3::common::Logger::WARNING);

    LoggerTestHelper helper;
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << helper << fts3::common::commit;
    EXPECT_EQ(helper.counter, 1);

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << helper << fts3::common::commit;
    EXPECT_EQ(helper.counter, 1);

    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << helper << fts3::common::commit;
    EXPECT_EQ(helper.counter, 2);

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << helper << fts3::common::commit;
    EXPECT_EQ(helper.counter, 2);

    logger.setLogLevel(fts3::common::Logger::DEBUG);
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << helper << fts3::common::commit;
    EXPECT_EQ(helper.counter, 3);
}


TEST(LogTest, Redirect) {
    const std::string logPath("/tmp/fts3tests.log");

    try {
        boost::filesystem::remove(logPath);
    }
    catch (...) {
        // Ignore
    }

    // Save current stdout and stderr
    int oldOut = dup(STDOUT_FILENO);
    int oldErr = dup(STDERR_FILENO);
    EXPECT_GT(oldOut, -1);
    EXPECT_GT(oldErr, -1);

    // Do the test
    fts3::common::Logger &logger = fts3::common::theLogger();
    EXPECT_EQ(logger.redirect(logPath, logPath), 0);

    logger.setLogLevel(fts3::common::Logger::WARNING);
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "APPEAR" << fts3::common::commit;
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "DOES NOT" << fts3::common::commit;

    std::ifstream read(logPath);

    bool appear = false;
    bool doesNot = false;

    while (!read.eof()) {
        char line[1024];
        read.getline(line, sizeof(line));
        if (strstr(line, "APPEAR")) {
            appear = true;
        }
        if (strstr(line, "DOES NOT")) {
            doesNot = true;
        }
    }

    EXPECT_TRUE(appear);
    EXPECT_TRUE(!doesNot);

    // Recover stdout and stderr
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    dup2(oldOut, STDOUT_FILENO);
    dup2(oldErr, STDERR_FILENO);
    close(oldOut);
    close(oldErr);

    // Clean
    EXPECT_NO_THROW(boost::filesystem::remove(logPath));
}
