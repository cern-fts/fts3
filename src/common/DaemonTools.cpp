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

#include <cstring>
#include <sys/resource.h>
#include <pwd.h>
#include <unistd.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <vector>


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
    getpwnam_r(name.c_str(), &pwbuf, buffer.data(), buffer.size(), &pwbufp);
    if (pwbufp == NULL) {
        throw SystemError("Could not get the UID for " + name);
    }
    return pwbufp->pw_uid;
}


int countProcessesWithName(const std::string& name)
{
    int count = 0;

    try {
        fs::directory_iterator end_itr;
        for (fs::directory_iterator itr("/proc"); itr != end_itr; ++itr) {
            char *endptr;
            strtol(itr->path().filename().c_str(), &endptr, 10);
            if (*endptr != '\0')
                continue;

            fs::path cmdline(itr->path());
            cmdline /= "cmdline";

            try {
                std::ifstream cmdlineStream(cmdline.file_string().c_str(), std::ios_base::in);
                char cmdName[256];

                cmdlineStream.getline(cmdName, sizeof(cmdName), '\0');

                if (boost::ends_with(cmdName, name)) {
                    ++count;
                }
            }
            catch (...) {
                // pass
            }
        }
    }
    catch (...) {
        return -1;
    }

    // At least, self must be counted!
    assert(count > 0);
    return count;
}


bool binaryExists(const std::string& name, std::string* fullPath)
{
    std::string envPath(getenv("PATH"));
    boost::char_separator<char> colon(":");
    boost::tokenizer<boost::char_separator<char>> tokenizer(envPath, colon);

    for (auto pathIter = tokenizer.begin(); pathIter != tokenizer.end(); ++pathIter) {
        fs::path path(*pathIter);
        path /= name;
        if (fs::exists(path) && access(path.file_string().c_str(), X_OK) == 0) {
            *fullPath = path.file_string();
            return true;
        }
    }

    return false;
}


} // namespace common
} // namespace fts3
