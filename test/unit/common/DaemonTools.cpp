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
#include <grp.h>
#include <pwd.h>
#include <fstream>

#include "common/DaemonTools.h"
#include "common/Exceptions.h"

using namespace fts3::common;

static void getCurrentUserAndGroup(std::string &user, std::string &group) {
    user = getpwuid(geteuid())->pw_name;
    group = getgrgid(getgid())->gr_name;
}

TEST(DaemonToolsTest, GetIdsTest) {
    EXPECT_EQ(getUserUid("root"), 0);
    EXPECT_EQ(getGroupGid("root"), 0);

    std::string user, group;
    getCurrentUserAndGroup(user, group);

    EXPECT_EQ(getUserUid(user), geteuid());
    EXPECT_EQ(getGroupGid(group), getegid());

    EXPECT_THROW(getUserUid("99user"), SystemError);
    EXPECT_THROW(getGroupGid("99group"), SystemError);
}


static std::string getCurrentProcName(void) {
    std::ifstream cmdline("/proc/self/cmdline");
    char cmd[512];
    cmdline.getline(cmd, sizeof(cmd), '\0');
    return cmd;
}


TEST(DaemonToolsTest, CountProcessesWithNameTest) {
    std::string self = getCurrentProcName();
    EXPECT_EQ(countProcessesWithName(self), 1);
    EXPECT_EQ(countProcessesWithName("/fake/path/really/unlikely"), 0);
}


TEST(DaemonToolsTest, BinaryExistsTest) {
    std::string path;

    EXPECT_TRUE(binaryExists("ls", &path));
    EXPECT_TRUE(!path.empty());
    EXPECT_TRUE(access(path.c_str(), F_OK) == 0);

    const char *oldPath = getenv("PATH");
    setenv("PATH", "/tmp", 1);

    EXPECT_TRUE(!binaryExists("ls", &path));

    setenv("PATH", oldPath, 1);
}


TEST(DaemonToolsTest, DropPrivilegesTest) {
    std::string user, group;
    getCurrentUserAndGroup(user, group);
    EXPECT_NO_THROW(dropPrivileges(user, group));
}
