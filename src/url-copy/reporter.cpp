/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "reporter.h"
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "boost/date_time/gregorian/gregorian.hpp"
#include <boost/tokenizer.hpp>
#include "producer_consumer_common.h"
#include <algorithm>
#include <ctime>
#include <sys/param.h>

using namespace std;

Reporter::Reporter(): nostreams(4), timeout(3600), buffersize(0), source_se(""), dest_se("") , msg(NULL), msg_updater(NULL), msg_log(NULL)
{
    msg = new struct message();
    msg_updater = new struct message_updater();
    msg_log = new struct message_log();
    char chname[MAXHOSTNAMELEN];
    gethostname(chname, sizeof(chname));
    hostname.assign(chname);
}

Reporter::~Reporter()
{
    if (msg)
        delete msg;
    if (msg_updater)
        delete msg_updater;
    if (msg_log)
        delete msg_log;
}

std::string Reporter::ReplaceNonPrintableCharacters(string s)
{
    try
        {
            std::string result("");
            for (size_t i = 0; i < s.length(); i++)
                {
                    char c = s[i];
                    int AsciiValue = static_cast<int> (c);
                    if (AsciiValue < 32 || AsciiValue > 127)
                        {
                            result.append(" ");
                        }
                    else
                        {
                            result += s.at(i);
                        }
                }
            return result;
        }
    catch (...)
        {
            return s;
        }
}

void Reporter::constructMessage(double throughput, bool retry, string job_id, string file_id, string transfer_status, string transfer_message, double timeInSecs, double filesize)
{
    try {
        msg->file_id  = boost::lexical_cast<unsigned int>(file_id);
    }
    catch (...) {
        return;
    }

    strncpy(msg->job_id, job_id.c_str(), sizeof(msg->job_id));
    strncpy(msg->transfer_status, transfer_status.c_str(), sizeof(msg->transfer_status));

    if(transfer_message.length() > 0 && transfer_message.length() >= 1023)
        {
            transfer_message = transfer_message.substr(0, 1023);
            transfer_message = ReplaceNonPrintableCharacters(transfer_message);
            strncpy(msg->transfer_message, transfer_message.c_str(), sizeof(msg->transfer_message));
        }
    else if(transfer_message.length() > 0 && transfer_message.length() < 1023)
        {
            transfer_message = ReplaceNonPrintableCharacters(transfer_message);
            strncpy(msg->transfer_message, transfer_message.c_str(), sizeof(msg->transfer_message));
        }
    else
        {
            memset(msg->transfer_message, 0, sizeof (msg->transfer_message));
        }

    msg->process_id = (int) getpid();
    msg->timeInSecs = timeInSecs;
    msg->filesize = filesize;
    msg->nostreams = nostreams;
    msg->timeout = timeout;
    msg->buffersize = buffersize;
    strncpy(msg->source_se, source_se.c_str(), sizeof(msg->source_se));
    strncpy(msg->dest_se, dest_se.c_str(), sizeof(msg->dest_se));
    msg->timestamp = milliseconds_since_epoch();
    msg->retry = retry;
    msg->throughput = throughput;

    // Try twice
    if (runProducerStatus(*msg) != 0)
        runProducerStatus(*msg);
}


void Reporter::constructMessageUpdater(std::string job_id, std::string file_id, double throughput, double transferred)
{
    try {
        msg_updater->file_id = boost::lexical_cast<unsigned int>(file_id);
    }
    catch (...) {
        return;
    }
    strncpy(msg_updater->job_id, job_id.c_str(), sizeof(msg_updater->job_id));
    msg_updater->process_id = (int) getpid();
    msg_updater->timestamp = milliseconds_since_epoch();
    msg_updater->throughput = throughput;
    msg_updater->transferred = transferred;
    // Try twice
    if (runProducerStall(*msg_updater) == 0)
        runProducerStall(*msg_updater);
}



void Reporter::constructMessageLog(std::string job_id, std::string file_id, std::string logFileName, bool debug)
{
    try {
        msg_log->file_id = boost::lexical_cast<unsigned int>(file_id);
    }
    catch (...) {
        return;
    }

    strncpy(msg_log->job_id, job_id.c_str(), sizeof(msg_log->job_id));
    strncpy(msg_log->filePath, logFileName.c_str(), sizeof(msg_log->filePath));
    strncpy(msg_log->host, hostname.c_str(), sizeof(msg_log->host));
    msg_log->debugFile = debug;
    msg_log->timestamp = milliseconds_since_epoch();
    // Try twice
    if (runProducerLog(*msg_log) == 0)
        runProducerLog(*msg_log);
}
