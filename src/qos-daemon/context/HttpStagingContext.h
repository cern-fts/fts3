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

#pragma once
#ifndef FTS3_HTTPSTAGINGCONTEXT_H
#define FTS3_HTTPSTAGINGCONTEXT_H

#include "StagingContext.h"

class HttpStagingContext : public StagingContext {

public:

    using JobContext::isValidOp;

    HttpStagingContext(QoSServer& qosServer, const StagingOperation& stagingOp) :
            StagingContext(qosServer, stagingOp)
    {}

    HttpStagingContext(const HttpStagingContext& copy) :
            StagingContext(copy), urlToMetadata(copy.urlToMetadata)
    {}
    HttpStagingContext(HttpStagingContext&& copy) :
            StagingContext(std::move(copy)), urlToMetadata(std::move(copy.urlToMetadata))
    {}

    void add(const StagingOperation& stagingOp) override;

    /**
    * For a given URL return the corresponding metadata
    * @param : url
    * @return : metadata
    */
    std::string getMetadata(const std::string& url) const;

private:
    /// URL -> Staging metadata
    std::map<std::string, std::string> urlToMetadata;
};


#endif //FTS3_HTTPSTAGINGCONTEXT_H
