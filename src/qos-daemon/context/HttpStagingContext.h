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

#include "StagingContext.h"

class HttpStagingContext : public StagingContext {

public:

    using JobContext::isValidOp;

    HttpStagingContext(QoSServer& qosServer, const StagingOperation& stagingOp) :
            StagingContext(qosServer, stagingOp), httpWaitingRoom(qosServer.getHttpWaitingRoom())
    {}

    HttpStagingContext(const HttpStagingContext& copy) :
            StagingContext(copy), urlToMetadata(copy.urlToMetadata), httpWaitingRoom(copy.httpWaitingRoom)
    {}
    HttpStagingContext(HttpStagingContext&& copy) :
            StagingContext(std::move(copy)), urlToMetadata(std::move(copy.urlToMetadata)), httpWaitingRoom(copy.httpWaitingRoom)
    {}

    void add(const StagingOperation& stagingOp) override;

    WaitingRoom<HttpPollTask>& getHttpWaitingRoom() {
        return httpWaitingRoom;
    }

    /**
    * For a given URL return the corresponding metadata
    * @param : url
    * @return : metadata
    */
    std::string getMetadata(const std::string& url) const;

private:
    /// URL -> Staging metadata
    std::map<std::string, std::string> urlToMetadata;
    WaitingRoom<HttpPollTask> &httpWaitingRoom;
};
