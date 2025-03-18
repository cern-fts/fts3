/*
 * Copyright (c) CERN 2013-2025
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

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "events.h"

struct DirQ;

class Consumer
{
private:
    std::string baseDir;
    unsigned limit;
    std::unique_ptr<DirQ> monitoringQueue;
    std::unique_ptr<DirQ> statusQueue;
    std::unique_ptr<DirQ> stalledQueue;
    std::unique_ptr<DirQ> logQueue;
    std::unique_ptr<DirQ> stagingQueue;
    std::unique_ptr<DirQ> deletionQueue;

public:

    Consumer(const std::string &baseDir, unsigned limit = 10000);
    ~Consumer();

    int runConsumerStatus(std::vector<fts3::events::Message> &messages);
    int runConsumerStall(std::vector<fts3::events::MessageUpdater> &messages);
    int runConsumerLog(std::map<int, fts3::events::MessageLog> &messages);
    int runConsumerDeletions(std::vector<fts3::events::MessageBringonline> &messages);
    int runConsumerStaging(std::vector<fts3::events::MessageBringonline> &messages);

    void purgeAll();
};
