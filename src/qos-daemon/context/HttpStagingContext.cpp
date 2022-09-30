/*
 * Copyright (c) CERN 2022
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

#include "HttpStagingContext.h"

// Only add unique surls to urlToIDs
void HttpStagingContext::add(const StagingOperation& stagingOp)
{
    bool validOp = isValidOp(stagingOp.surl, stagingOp.jobId, stagingOp.fileId);

    if (validOp && !urlToIDs.count(stagingOp.surl)) {
        urlToMetadata.insert({stagingOp.surl, stagingOp.stagingMetadata});
        StagingContext::add(stagingOp);
    }
}


std::string HttpStagingContext::getMetadata(const std::string& url) const
{
    auto it = urlToMetadata.find(url);

    if (it != urlToMetadata.end()) {
        return it->second;
    }

    return "";
}
