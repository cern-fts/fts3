/*
 * Copyright (c) CERN 2015
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

#include <grp.h>
#include <pwd.h>

#include <boost/test/included/unit_test.hpp>

#include "common/DaemonTools.h"
#include "common/Exceptions.h"

using namespace fts3::common;


static void getCurrentUserAndGroup(std::string &user, std::string &group)
{
    user = getpwuid(geteuid())->pw_name;
    group = getgrgid(getgid())->gr_name;
}


BOOST_AUTO_TEST_SUITE(common)
BOOST_AUTO_TEST_SUITE(DaemonToolsTest)


BOOST_AUTO_TEST_CASE(getIdsTest)
{
    BOOST_CHECK_EQUAL(getUserUid("root"), 0);
    BOOST_CHECK_EQUAL(getGroupGid("root"), 0);

    std::string user, group;
    getCurrentUserAndGroup(user, group);

    BOOST_CHECK_EQUAL(getUserUid(user), geteuid());
    BOOST_CHECK_EQUAL(getGroupGid(group), getegid());

    BOOST_CHECK_THROW(getUserUid("99user"), SystemError);
    BOOST_CHECK_THROW(getGroupGid("99group"), SystemError);
}


BOOST_AUTO_TEST_CASE(countProcessesWithNameTest)
{
    BOOST_CHECK_EQUAL(countProcessesWithName("init"), 1);
    BOOST_CHECK_EQUAL(countProcessesWithName("/fake/path/really/unlikely"), 0);
}


BOOST_AUTO_TEST_CASE(binaryExistsTest)
{
    std::string path;

    BOOST_CHECK(binaryExists("ls", &path));
    BOOST_CHECK(!path.empty());
    BOOST_CHECK(access(path.c_str(), F_OK) == 0);

    const char* oldPath = getenv("PATH");
    setenv("PATH", "/tmp", 1);

    BOOST_CHECK(!binaryExists("ls", &path));

    setenv("PATH", oldPath, 1);
}


BOOST_AUTO_TEST_CASE(dropPrivilegesTest)
{
    std::string user, group;
    getCurrentUserAndGroup(user, group);
    BOOST_CHECK_NO_THROW(dropPrivileges(user, group));

    BOOST_CHECK_THROW(dropPrivileges("//***//", "//****//"), SystemError);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
