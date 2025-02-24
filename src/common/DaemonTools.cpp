/*
 * Copyright (c) CERN 2013-2015
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

#include "DaemonTools.h"
#include "Exceptions.h"

#include <charconv>
#include <cstring>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <filesystem>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <vector>
#include <iostream>


namespace fs = boost::filesystem;


namespace fts3 {
namespace common {

// courtesy of:
// "Setuid Demystified" by Hao Chen, David Wagner, and Drew Dean: http://www.cs.berkeley.edu/~daw/papers/setuid-usenix02.pdf
uid_t getUserUid(const std::string& name)
{
    long buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (buflen == -1) {
        buflen = 64;
    }

    std::vector<char> buffer(buflen);
    struct passwd pwbuf, *pwbufp;
    if (getpwnam_r(name.c_str(), &pwbuf, buffer.data(), buffer.size(), &pwbufp) < 0 || pwbufp == NULL) {
        throw SystemError("Could not get the UID for " + name);
    }
    return pwbufp->pw_uid;
}


uid_t getGroupGid(const std::string& name)
{
    long buflen = sysconf(_SC_GETGR_R_SIZE_MAX);
    if (buflen == -1) {
        buflen = 64;
    }

    std::vector<char> buffer(buflen);
    struct group grpbuf, *grpbufp;
    if (getgrnam_r(name.c_str(), &grpbuf, buffer.data(), buffer.size(), &grpbufp) < 0 || grpbufp == NULL) {
        throw SystemError("Could not get the GID for " + name);
    }
    return grpbufp->gr_gid;
}


int countProcessesWithName(const std::string& name)
{
    int count = 0;

    try {
        const std::filesystem::path proc{"/proc"};
        for (auto const& proc_entry : std::filesystem::directory_iterator{proc}) {
            std::filesystem::path pid = proc_entry.path();
            int result;
            auto [ptr, ec] = std::from_chars(pid.filename().c_str(), pid.filename().c_str() + pid.filename().string().size(), result);
            if (ec != std::errc())
                continue;

            const std::filesystem::path cmdline{pid.string() + "/cmdline"};
            try {
                std::ifstream cmdlineStream(cmdline.c_str(), std::ios_base::in);
                std::string cmdName;

                std::getline(cmdlineStream, cmdName, '\0');
                if (cmdName.ends_with(name)) {
                    ++count;
                }
            } catch (...) {
                // pass
            }
       }
    } catch (...) {
        return -1;
    }

    return count;
}


bool binaryExists(const std::string& name, std::string* fullPath)
{
    const char* envC = getenv("PATH");
    if (envC == NULL) {
        return false;
    }

    std::string envPath(envC);
    boost::char_separator<char> colon(":");
    boost::tokenizer<boost::char_separator<char>> tokenizer(envPath, colon);

    for (auto pathIter = tokenizer.begin(); pathIter != tokenizer.end(); ++pathIter) {
        fs::path path(*pathIter);
        path /= name;
        if (fs::exists(path) && access(path.string().c_str(), X_OK) == 0) {
            *fullPath = path.string();
            return true;
        }
    }

    return false;
}


bool dropPrivileges(const std::string& user, const std::string& group)
{
    if (geteuid() != 0 || getegid() != 0) {
        return false;
    }

    uid_t uid = getUserUid(user);
    gid_t gid = getGroupGid(group);

    if (setgid(gid) < 0)
        throw SystemError("Could not change the GID");
    if (setegid(gid) < 0)
        throw SystemError("Could not change the effective GID");
    if (setuid(uid) < 0)
        throw SystemError("Could not change the UID");
    if (seteuid(uid) < 0)
        throw SystemError("Could not change the effective UID");

    return true;
}


} // namespace common
} // namespace fts3
