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
#include "error.h"

#include <fstream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace fts3 {
namespace common {


int countProcessesWithName(const std::string &name)
{
    int count = 0;

    try {
        fs::directory_iterator end_itr;
        for (fs::directory_iterator itr("/proc"); itr != end_itr; ++itr) {
            char *endptr;
            errno = 0;
            long pid = strtol(itr->path().filename().c_str(), &endptr, 10);
            if (*endptr != '\0' || ((pid == LONG_MIN || pid == LONG_MAX) && errno == ERANGE))
                continue;

            fs::path cmdline(itr->path());
            cmdline /= "cmdline";

            try {
                std::ifstream cmdlineStream(cmdline.string().c_str(), std::ios_base::in);
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

    return count;
}

}
}

