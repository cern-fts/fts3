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
#include "boost/date_time/gregorian/gregorian.hpp"
#include "common/definitions.h"
#include "common/Uri.h"

using namespace std;


Reporter::Reporter(const std::string &msgDir): nostreams(4), timeout(3600), buffersize(0),
    producer(msgDir), isTerminalSent(false), multiple(false)
{
    memset(&msg, 0, sizeof(Message));
    memset(&msg_updater, 0, sizeof(MessageUpdater));
    memset(&msg_log, 0, sizeof(MessageLog));

    hostname = fts3::common::getFullHostname();
}


Reporter::~Reporter()
{
}


std::string Reporter::ReplaceNonPrintableCharacters(string s)
{
    try {
        std::string result("");
        for (size_t i = 0; i < s.length(); i++) {
            char c = s[i];
            int AsciiValue = static_cast<int> (c);
            if (AsciiValue < 32 || AsciiValue > 127) {
                result.append(" ");
            }
            else {
                result += s.at(i);
            }
        }
        return result;
    }
    catch (...) {
        return s;
    }
}


void Reporter::sendMessage(double throughput, bool retry,
                           const string &job_id, unsigned file_id,
                           const string &transfer_status, const string &transfer_message,
                           double timeInSecs, off_t filesize)
{
    msg.file_id = file_id;
    g_strlcpy(msg.job_id, job_id.c_str(), sizeof(msg.job_id));
    g_strlcpy(msg.transfer_status, transfer_status.c_str(), sizeof(msg.transfer_status));

    if (transfer_message.length() > 0) {
        std::string trmsg(transfer_message);
        if (trmsg.length() >= 1023) {
            trmsg = trmsg.substr(0, 1023);
        }
        trmsg = ReplaceNonPrintableCharacters(trmsg);
        g_strlcpy(msg.transfer_message, trmsg.c_str(), sizeof(msg.transfer_message));
    }
    else {
        msg.transfer_message[0] = '\0';
    }

    msg.process_id = (int) getpid();
    msg.timeInSecs = timeInSecs;
    msg.filesize = (double) filesize;
    msg.nostreams = nostreams;
    msg.timeout = timeout;
    msg.buffersize = buffersize;
    g_strlcpy(msg.source_se, source_se.c_str(), sizeof(msg.source_se));
    g_strlcpy(msg.dest_se, dest_se.c_str(), sizeof(msg.dest_se));
    msg.timestamp = milliseconds_since_epoch();
    msg.retry = retry;
    msg.throughput = throughput;

    // Try twice
    if (producer.runProducerStatus(msg) != 0) {
        producer.runProducerStatus(msg);
    }
}


void Reporter::sendTerminal(double throughput, bool retry,
    const string &job_id, unsigned file_id,
    const string &transfer_status, const string &transfer_message,
    double timeInSecs, off_t filesize)
{
    // Did we send it already?
    if (!multiple) {
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


void Reporter::sendPing(const std::string &job_id, unsigned file_id,
    double throughput, off_t transferred,
    std::string source_surl, std::string dest_surl, std::string source_turl,
    std::string dest_turl, const std::string &transfer_status)
{
    g_strlcpy(msg_updater.job_id, job_id.c_str(), sizeof(msg_updater.job_id));
    msg_updater.file_id = file_id;
    msg_updater.process_id = (int) getpid();
    msg_updater.timestamp = milliseconds_since_epoch();
    msg_updater.throughput = throughput;
    msg_updater.transferred = (double) transferred;

    g_strlcpy(msg_updater.source_surl, source_surl.c_str(), sizeof(msg_updater.source_surl));
    g_strlcpy(msg_updater.dest_surl, dest_surl.c_str(), sizeof(msg_updater.dest_surl));
    g_strlcpy(msg_updater.source_turl, source_turl.c_str(), sizeof(msg_updater.source_turl));
    g_strlcpy(msg_updater.dest_turl, dest_turl.c_str(), sizeof(msg_updater.dest_turl));
    g_strlcpy(msg_updater.transfer_status, transfer_status.c_str(), sizeof(msg_updater.transfer_status));

    // Try twice
    if (producer.runProducerStall(msg_updater) != 0) {
        producer.runProducerStall(msg_updater);
    }
}


void Reporter::sendLog(const std::string &job_id, unsigned file_id,
    const std::string &logFileName, bool debug)
{
    msg_log.file_id = file_id;
    g_strlcpy(msg_log.job_id, job_id.c_str(), sizeof(msg_log.job_id));
    g_strlcpy(msg_log.filePath, logFileName.c_str(), sizeof(msg_log.filePath));
    g_strlcpy(msg_log.host, hostname.c_str(), sizeof(msg_log.host));
    msg_log.debugFile = debug;
    msg_log.timestamp = milliseconds_since_epoch();
    // Try twice
    if (producer.runProducerLog(msg_log) != 0) {
        producer.runProducerLog(msg_log);
    }
}
