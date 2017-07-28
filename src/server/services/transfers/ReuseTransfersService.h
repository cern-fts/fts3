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
#ifndef PROCESSSERVICEREUSE_H_
#define PROCESSSERVICEREUSE_H_

#include "TransfersService.h"
#include "UrlCopyCmd.h"


namespace fts3 {
namespace server {


class ReuseTransfersService: public TransfersService
{
public:
    ReuseTransfersService();
    virtual void runService();

protected:
    void writeJobFile(const std::string& jobId, const std::vector<std::string>& files);
    std::map<uint64_t, std::string> generateJobFile(const std::string& jobId, const std::list<TransferFile>& files);
    void getFiles(const std::vector<QueueId>& queues);
    void startUrlCopy(const std::string& jobId, const std::list<TransferFile>& files);
    void executeUrlcopy();
};

} // end namespace server
} // end namespace fts3

#endif // PROCESSSERVICEREUSE_H_
