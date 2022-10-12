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

#include "services/BaseService.h"
#include "services/heartbeat/HeartBeat.h"

namespace fts3 {
namespace server {

class ForceStartTransferService: public BaseService
{
public:
    ForceStartTransferService(HeartBeat *beat);
    virtual void runService();

protected:
    std::string ftsHostName;
    std::string infosys;
    bool monitoringMessages;
    int execPoolSize;
    std::string logDir;
    std::string msgDir;
    boost::posix_time::time_duration pollInterval;

    HeartBeat *beat;
    void forceRunJobs();
};

} // end namespace server
} // end namespace fts3
