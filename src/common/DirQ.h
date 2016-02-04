/*
 * Copyright (c) CERN 2016
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

#ifndef DIRQ_H 
#define DIRQ_H

#include <dirq.h>
#include <sstream>
#include "common/Exceptions.h"

class DirQ {
private:
    dirq_t dirq;
    std::string path;

public:
    DirQ(const std::string &path): path(path) {
        dirq = dirq_new(path.c_str());
        if (dirq_get_errcode(dirq)) {
            std::stringstream msg;
            msg << "Could not create dirq instance for " << path
                << "(" << dirq_get_errstr(dirq) << ")";
            throw fts3::common::SystemError(msg.str());
        }
    }

    ~DirQ() {
        dirq_free(dirq);
        dirq = NULL;
    }

    DirQ(const DirQ&) = delete;
    DirQ& operator = (DirQ&) = delete;

    const std::string &getPath(void) const throw() {
        return path;
    }

    operator dirq_t () {
        return dirq;
    }
};

#endif //DIRQ_H
