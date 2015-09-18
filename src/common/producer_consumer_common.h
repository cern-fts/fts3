/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

#pragma once

#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include "definitions.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

#define STALLED_DIR "/var/lib/fts3/stalled/"
#define MONITORING_DIR "/var/lib/fts3/monitoring/"
#define STATUS_DIR "/var/lib/fts3/status/"
#define LOG_DIR "/var/lib/fts3/logs/"
#define STATUS_DM_DIR "/var/lib/fts3/status/"
#define DEFAULT_LIMIT 10000

int getDir(const std::string& dir, std::vector<std::string> &files,
        const std::string& extension, unsigned limit);

void getUniqueTempFileName(const std::string& basename, std::string& tempname);

void mktempfile(const std::string& basename, std::string& tempname);

//for monitoring
int runConsumerMonitoring(std::vector<struct message_monitoring>& messages,
        unsigned limit=DEFAULT_LIMIT);

int runProducerMonitoring(struct message_monitoring &msg);


//for receiving the status from url_copy
int runConsumerStatus(std::vector<struct message>& messages,
        unsigned limit=DEFAULT_LIMIT);

int runProducerStatus(struct message &msg);


//for checking is the process is stalled
int runConsumerStall(std::vector<struct message_updater>& messages,
        unsigned limit = DEFAULT_LIMIT);

int runProducerStall(struct message_updater &msg);


//for checking the log file path
int runConsumerLog(std::map<int, struct message_log>& messages, unsigned limit =
        DEFAULT_LIMIT);

int runProducerLog(struct message_log &msg);

//for staging and deletion recovery
int runConsumerDeletions(std::vector<struct message_bringonline>& messages,
        unsigned limit = DEFAULT_LIMIT);
int runProducerDeletions(struct message_bringonline &msg);

int runConsumerStaging(std::vector<struct message_bringonline>& messages,
        unsigned limit = DEFAULT_LIMIT);
int runProducerStaging(struct message_bringonline &msg);

int runProducer(struct message_bringonline &msg, std::string const & operation);


boost::posix_time::time_duration::tick_type milliseconds_since_epoch();
