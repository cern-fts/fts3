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

#include <glib.h>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "common/definitions.h"
#include "common/Uri.h"

using namespace std;


Reporter::Reporter(const std::string &msgDir): nostreams(4), timeout(3600), buffersize(0),
    producer(msgDir), isTerminalSent(false), multiple(false)
{
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
    msg.set_file_id(file_id);
    msg.set_job_id(job_id);
    msg.set_transfer_status(transfer_status);
    msg.set_transfer_message(ReplaceNonPrintableCharacters(transfer_message));
    msg.set_process_id(getpid());
    msg.set_time_in_secs(timeInSecs);
    msg.set_filesize(filesize);
    msg.set_nostreams(nostreams);
    msg.set_timeout(timeout);
    msg.set_buffersize(buffersize);
    msg.set_source_se(source_se);
    msg.set_dest_se(dest_se);
    msg.set_timestamp(milliseconds_since_epoch());
    msg.set_retry(retry);
    msg.set_throughput(throughput);

    producer.runProducerStatus(msg);
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
    msg_updater.set_job_id(job_id);
    msg_updater.set_file_id(file_id);
    msg_updater.set_process_id(getpid());
    msg_updater.set_timestamp(milliseconds_since_epoch());
    msg_updater.set_throughput(throughput);
    msg_updater.set_transferred(transferred);

    msg_updater.set_source_surl(source_surl);
    msg_updater.set_dest_surl(dest_surl);
    msg_updater.set_source_turl(source_turl);
    msg_updater.set_dest_turl(dest_turl);
    msg_updater.set_transfer_status(transfer_status);

    producer.runProducerStall(msg_updater);
}


void Reporter::sendLog(const std::string &job_id, unsigned file_id,
    const std::string &logFileName, bool debug)
{
    msg_log.set_file_id(file_id);
    msg_log.set_job_id(job_id);
    msg_log.set_log_path(logFileName);
    msg_log.set_host(hostname);
    msg_log.set_has_debug_file(debug);
    msg_log.set_timestamp(milliseconds_since_epoch());

    producer.runProducerLog(msg_log);
}
