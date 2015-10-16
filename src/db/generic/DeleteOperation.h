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
#ifndef NAMESPACEOPERATION_H_
#define DELETEOPERATION_H_

#include <stdint.h>
#include <string>


struct DeleteOperation {
    DeleteOperation(const std::string& jobId, uint64_t fileId, const std::string& voName,
        const std::string& userDn, const std::string& credId, const std::string& url) :
        jobId(jobId), fileId(fileId), voName(voName), userDn(userDn), credId(credId), url(url)
    {
    }

    std::string jobId;
    uint64_t    fileId;
    std::string voName;
    std::string userDn;
    std::string credId;
    std::string url;
};


#endif // NAMESPACEOPERATION_H_
