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
#ifndef QosTransitionOperation_H_
#define QosTransitionOperation_H_

#include <stdint.h>
#include <string>


struct QosTransitionOperation {
	QosTransitionOperation(const std::string& jobId, uint64_t fileId, const std::string& surl, const std::string& target_qos, const std::string& token):
        jobId(jobId), fileId(fileId), surl(surl), target_qos(target_qos), token(token)
    {}

    inline bool valid() const {
	    return (!surl.empty() && !jobId.empty() && !target_qos.empty() && !token.empty());
	}

    // Required so it can be used as a key in std::containers
    inline bool operator < (const QosTransitionOperation& that) const {
        return surl < that.surl;
    }

    std::string jobId;
    uint64_t fileId;
    std::string surl;
    std::string target_qos;
    std::string token; ///< IAM token
};


#endif // QosTransitionOperation
