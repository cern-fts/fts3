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

#ifndef MINFILESTATUS_H_
#define MINFILESTATUS_H_

#include <stdint.h>
#include <string>


struct MinFileStatus {
    MinFileStatus(const std::string& jobId, uint64_t fileId, const std::string& state,
        const std::string& reason, bool retry) :
        jobId(jobId), fileId(fileId), state(state), reason(reason), retry(retry)
    {
    }

    std::string jobId;
    uint64_t fileId;
    std::string state;
    std::string reason;
    bool retry;
};


#endif //MINFILESTATUS_H_
