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

#include <ctime>
#include <iostream>
#include <boost/version.hpp>

#ifdef __STDC_NO_ATOMICS__
#include <atomic>
#else
#if BOOST_VERSION >= 105300
#include <boost/atomic.hpp>
#else
#include <stdatomic.h>
#endif
#endif

#include <boost/thread/recursive_mutex.hpp>

#include "msg-bus/producer.h"


class Reporter
{

public:
    Reporter(const std::string &msgDir);
    ~Reporter();

    // Send to the server a message with the current status
    void sendMessage(double throughput, bool retry,
                     const std::string& job_id, unsigned file_id,
                     const std::string& transfer_status, const std::string& transfer_message,
                     double timeInSecs,  off_t fileSize);

    // Same as before, but it can be run only once!
    void sendTerminal(double throughput, bool retry,
                      const std::string& job_id, unsigned file_id,
                      const std::string& transfer_status, const std::string& transfer_message,
                      double timeInSecs,  off_t fileSize);

    // Let the server know that we are alive, and how are we doing
    void sendPing(const std::string& job_id, unsigned file_id,
                  double throughput, off_t transferred,
                  std::string source_surl, std::string dest_surl,std::string source_turl, std::string dest_turl, const std::string& transfer_status);

    // Send to the server the log file
    void sendLog(const std::string& job_id, unsigned file_id,
                 const std::string& logFileName, bool debug);

    unsigned int nostreams;
    unsigned int timeout;
    unsigned int buffersize;
    std::string source_se;
    std::string dest_se;
    static std::string ReplaceNonPrintableCharacters(std::string s);
    void setMultipleTransfers(bool state)
    {
        multiple = state;
    }

    Producer producer;

private:
    struct Message         msg;
    struct MessageUpdater msg_updater;
    struct MessageLog     msg_log;
    std::string            hostname;

#ifdef __STDC_NO_ATOMICS__
    std::atomic<bool> isTerminalSent;
#else
#if BOOST_VERSION >= 105300
    boost::atomic<bool> isTerminalSent;
#else
    std::atomic_bool isTerminalSent;
#endif
#endif

    bool multiple;
};
