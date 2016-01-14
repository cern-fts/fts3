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
#ifndef CONSUMER_H
#define CONSUMER_H

#include <string>
#include <vector>
#include "DirQ.h"
#include "messages.h"


class Consumer
{
private:
    std::string baseDir;
    unsigned limit;
    DirQ monitoringQueue;
    DirQ statusQueue;
    DirQ stalledQueue;
    DirQ logQueue;
    DirQ stagingQueue;
    DirQ deletionQueue;

public:

    Consumer(const std::string &baseDir, unsigned limit = 10000);

    ~Consumer();

    int runConsumerMonitoring(std::vector<struct MessageMonitoring> &messages);

    int runConsumerStatus(std::vector<struct Message> &messages);

    int runConsumerStall(std::vector<struct MessageUpdater> &messages);

    int runConsumerLog(std::map<int, struct MessageLog> &messages);

    int runConsumerDeletions(std::vector<struct MessageBringonline> &messages);

    int runConsumerStaging(std::vector<struct MessageBringonline> &messages);

    void purgeAll();
};


#endif // CONSUMER_H
