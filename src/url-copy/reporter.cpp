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

#include "reporter.h"
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "boost/date_time/gregorian/gregorian.hpp"
#include <boost/tokenizer.hpp>
#include "common/producer_consumer_common.h"
#include <algorithm>
#include <ctime>
#include <sys/param.h>

using namespace std;

Reporter::Reporter(): nostreams(4), timeout(3600), buffersize(0),
    source_se(""), dest_se(""),
    msg(NULL), msg_updater(NULL), msg_log(NULL),
    isTerminalSent(false), multiple(false)
{
    msg = new struct message();
    memset(msg, 0, sizeof(message));
    msg_updater = new struct message_updater();
    memset(msg_updater, 0, sizeof(message_updater));
    msg_log = new struct message_log();
    memset(msg_log, 0, sizeof(message_log));

    char chname[MAXHOSTNAMELEN] = {0};
    gethostname(chname, sizeof(chname));
    hostname.assign(chname);
}

Reporter::~Reporter()
{
    delete msg;
    delete msg_updater;
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

void Reporter::sendMessage(double throughput, bool retry,
                           const string& job_id, unsigned file_id,
                           const string& transfer_status, const string& transfer_message,
                           double timeInSecs, off_t filesize)
{
    msg->file_id  = file_id;
    strncpy(msg->job_id, job_id.c_str(), sizeof(msg->job_id));
    msg->job_id[sizeof(msg->job_id) -1] = '\0';
    strncpy(msg->transfer_status, transfer_status.c_str(), sizeof(msg->transfer_status));
    msg->transfer_status[sizeof(msg->transfer_status) -1] = '\0';

    if (transfer_message.length() > 0)
        {
            std::string trmsg(transfer_message);
            if (trmsg.length() >= 1023) {
                trmsg = trmsg.substr(0, 1023);
            }
            trmsg = ReplaceNonPrintableCharacters(trmsg);
            strncpy(msg->transfer_message, trmsg.c_str(), sizeof(msg->transfer_message));
            msg->transfer_message[sizeof(msg->transfer_message) -1] = '\0';
        }
    else
        {
            msg->transfer_message[0] = '\0';
        }

    msg->process_id = (int) getpid();
    msg->timeInSecs = timeInSecs;
    msg->filesize = (double)filesize;
    msg->nostreams = nostreams;
    msg->timeout = timeout;
    msg->buffersize = buffersize;
    strncpy(msg->source_se, source_se.c_str(), sizeof(msg->source_se));
    msg->source_se[sizeof(msg->source_se) -1] = '\0';
    strncpy(msg->dest_se, dest_se.c_str(), sizeof(msg->dest_se));
    msg->dest_se[sizeof(msg->dest_se) -1] = '\0';
    msg->timestamp = milliseconds_since_epoch();
    msg->retry = retry;
    msg->throughput = throughput;

    // Try twice
    if (runProducerStatus(*msg) != 0) {
        runProducerStatus(*msg);
    }
}

void Reporter::sendTerminal(double throughput, bool retry,
                            const string& job_id, unsigned file_id,
                            const string& transfer_status, const string& transfer_message,
                            double timeInSecs, off_t filesize)
{
    // Did we send it already?
    if (!multiple)
        {
            if (isTerminalSent) {
                return;
            }
            isTerminalSent = true;
        }
    // Not sent, so send it now and raise the flag
    sendMessage(throughput, retry, job_id, file_id,
                transfer_status, transfer_message,
                timeInSecs, filesize);

}

void Reporter::sendPing(const std::string& job_id, unsigned file_id,
                        double throughput, off_t transferred,
                        std::string source_surl, std::string dest_surl,std::string source_turl,
                        std::string dest_turl, const std::string& transfer_status)
{
    strncpy(msg_updater->job_id, job_id.c_str(), sizeof(msg_updater->job_id));
    msg_updater->job_id[sizeof(msg_updater->job_id) -1] = '\0';
    msg_updater->file_id = file_id;
    msg_updater->process_id = (int) getpid();
    msg_updater->timestamp = milliseconds_since_epoch();
    msg_updater->throughput = throughput;
    msg_updater->transferred = (double)transferred;

    strncpy(msg_updater->source_surl, source_surl.c_str(), sizeof(msg_updater->source_surl));
    msg_updater->source_surl[sizeof(msg_updater->source_surl) -1] = '\0';

    strncpy(msg_updater->dest_surl, dest_surl.c_str(), sizeof(msg_updater->dest_surl));
    msg_updater->dest_surl[sizeof(msg_updater->dest_surl) -1] = '\0';

    strncpy(msg_updater->source_turl, source_turl.c_str(), sizeof(msg_updater->source_turl));
    msg_updater->source_turl[sizeof(msg_updater->source_turl) -1] = '\0';

    strncpy(msg_updater->dest_turl, dest_turl.c_str(), sizeof(msg_updater->dest_turl));
    msg_updater->dest_turl[sizeof(msg_updater->dest_turl) -1] = '\0';

    strncpy(msg_updater->transfer_status, transfer_status.c_str(), sizeof(msg_updater->transfer_status));
    msg_updater->transfer_status[sizeof(msg_updater->transfer_status) -1] = '\0';

    // Try twice
    if (runProducerStall(*msg_updater) != 0) {
        runProducerStall(*msg_updater);
    }
}



void Reporter::sendLog(const std::string& job_id, unsigned file_id,
                       const std::string& logFileName, bool debug)
{
    msg_log->file_id = file_id;
    strncpy(msg_log->job_id, job_id.c_str(), sizeof(msg_log->job_id));
    msg_log->job_id[sizeof(msg_log->job_id) -1] = '\0';
    strncpy(msg_log->filePath, logFileName.c_str(), sizeof(msg_log->filePath));
    msg_log->filePath[sizeof(msg_log->filePath) -1] = '\0';
    strncpy(msg_log->host, hostname.c_str(), sizeof(msg_log->host));
    msg_log->host[sizeof(msg_log->host) -1] = '\0';
    msg_log->debugFile = debug;
    msg_log->timestamp = milliseconds_since_epoch();
    // Try twice
    if (runProducerLog(*msg_log) != 0) {
        runProducerLog(*msg_log);
    }
}
