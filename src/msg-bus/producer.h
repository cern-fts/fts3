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
#ifndef PRODUCER_H
#define PRODUCER_H

#include <memory>
#include <string>
#include "events.h"

struct DirQ;

class Producer
{
private:
    std::string baseDir;
    std::unique_ptr<DirQ> monitoringQueue;
    std::unique_ptr<DirQ> statusQueue;
    std::unique_ptr<DirQ> stalledQueue;
    std::unique_ptr<DirQ> logQueue;
    std::unique_ptr<DirQ> deletionQueue;
    std::unique_ptr<DirQ> stagingQueue;
    std::unique_ptr<DirQ> eventsQueue;

public:
    Producer(const std::string &baseDir);

    ~Producer();

    int runProducerStatus(const fts3::events::Message &msg);

    int runProducerLog(const fts3::events::MessageLog &msg);

    int runProducerDeletions(const fts3::events::MessageBringonline &msg);

    int runProducerStaging(const fts3::events::MessageBringonline &msg);

    int runProducerMonitoring(const std::string &serialized);

    int runProducerEvents(const std::string &serialized);
};

#endif // PRODUCER_H
