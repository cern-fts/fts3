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

int getDir (std::string dir, std::vector<std::string> &files);

void getUniqueTempFileName(const std::string& basename, std::string& tempname);

void mktempfile(const std::string& basename, std::string& tempname);

//for monitoring
void runConsumerMonitoring(std::vector<struct message_monitoring>& messages);

void runProducerMonitoring(struct message_monitoring);


//for receiving the status from url_copy
void runConsumerStatus(std::vector<struct message>& messages);

void runProducerStatus(struct message msg);


//for checking is the process is stalled
void runConsumerStall(std::vector<struct message_updater>& messages);

void runProducerStall(struct message_updater msg);


//for checking the log file path
void runConsumerLog(std::vector<struct message_log>& messages);

void runProducerLog(struct message_log msg);



boost::posix_time::time_duration::tick_type milliseconds_since_epoch();
