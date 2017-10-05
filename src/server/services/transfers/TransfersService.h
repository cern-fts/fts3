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

#pragma once
#ifndef PROCESSSERVICE_H_
#define PROCESSSERVICE_H_

#include <string>
#include <vector>

#include "db/generic/QueueId.h"
#include "../BaseService.h"


namespace fts3 {
namespace server {


class TransfersService: public BaseService
{
public:
    /// Constructor
    TransfersService();

    /// Destructor
    virtual ~TransfersService();

    virtual void runService();

protected:
    std::string ftsHostName;
    std::string infosys;
    bool monitoringMessages;
    int execPoolSize;
    std::string cmd;
    std::string logDir;
    std::string msgDir;
    boost::posix_time::time_duration schedulingInterval;

    void getFiles(const std::vector<QueueId>& queues, int availableUrlCopySlots);
    void executeUrlcopy();
};

} // end namespace server
} // end namespace fts3

#endif // PROCESSSERVICE_H_
