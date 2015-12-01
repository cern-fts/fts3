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
#ifndef PRODUCERCONSUMERCOMMON_H
#define PRODUCERCONSUMERCOMMON_H

#include "messages.h"


#define DEFAULT_LIMIT 10000

int getDir(const std::string& dir, std::vector<std::string> &files,
        const std::string& extension, unsigned limit);

void getUniqueTempFileName(const std::string& basename, std::string& tempname);

void mktempfile(const std::string& basename, std::string& tempname);

//for monitoring
int runConsumerMonitoring(std::vector<struct MessageMonitoring>& messages,
        unsigned limit=DEFAULT_LIMIT);

int runProducerMonitoring(const struct MessageMonitoring &msg);


//for receiving the status from url_copy
int runConsumerStatus(std::vector<struct Message>& messages,
        unsigned limit=DEFAULT_LIMIT);

int runProducerStatus(const struct Message &msg);


//for checking is the process is stalled
int runConsumerStall(std::vector<struct MessageUpdater>& messages,
        unsigned limit = DEFAULT_LIMIT);

int runProducerStall(const struct MessageUpdater &msg);


//for checking the log file path
int runConsumerLog(std::map<int, struct MessageLog>& messages, unsigned limit =
        DEFAULT_LIMIT);

int runProducerLog(const struct MessageLog &msg);

//for staging and deletion recovery
int runConsumerDeletions(std::vector<struct MessageBringonline>& messages,
        unsigned limit = DEFAULT_LIMIT);
int runProducerDeletions(const struct MessageBringonline &msg);

int runConsumerStaging(std::vector<struct MessageBringonline>& messages,
        unsigned limit = DEFAULT_LIMIT);
int runProducerStaging(const struct MessageBringonline &msg);

int runProducer(const struct MessageBringonline &msg, std::string const & operation);


#endif // PRODUCERCONSUMERCOMMON_H
