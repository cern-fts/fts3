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

#pragma once
#ifndef ARCHIVINGOPERATION_H_
#define ARCHIVINGOPERATION_H_

#include <stdint.h>
#include <string>


struct ArchivingOperation {
	ArchivingOperation(const std::string& jobId, uint64_t fileId, const std::string& voName,
        const std::string& user, const std::string& credId, const std::string& surl,
       time_t timeout):
        jobId(jobId), fileId(fileId), voName(voName), user(user), credId(credId), surl(surl),
        timeout(timeout)
    {
    }

    std::string jobId;
    uint64_t fileId;
    std::string voName;
    std::string user;
    std::string credId;
    std::string surl;
    time_t timeout;
};


#endif // ARCHIVINGOPERATION_H_
