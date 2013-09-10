/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***********************************************/


#pragma once

#include <ctime>
#include <iostream>
#ifdef __STDC_NO_ATOMICS__
#include <atomic>
#else
#include <stdatomic.h>
#endif

class Reporter
{

public:
    Reporter();
    ~Reporter();

    // Send to the server a message with the current status
    void sendMessage(double throughput, bool retry,
                     const std::string& job_id, const std::string& file_id,
                     const std::string& transfer_status, const std::string& transfer_message,
                     double timeInSecs,  double fileSize);

    // Same as before, but it can be run only once!
    void sendTerminal(double throughput, bool retry,
                      const std::string& job_id, const std::string& file_id,
                      const std::string& transfer_status, const std::string& transfer_message,
                      double timeInSecs,  double fileSize);

    // Let the server know that we are alive, and how are we doing
    void sendPing(const std::string& job_id, const std::string& file_id,
                  double throughput, double transferred);

    // Send to the server the log file
    void sendLog(const std::string& job_id, const std::string& file_id,
                 const std::string& logFileName, bool debug);

    unsigned int nostreams;
    unsigned int timeout;
    unsigned int buffersize;
    std::string source_se;
    std::string dest_se;
    static std::string ReplaceNonPrintableCharacters(std::string s);
    void setReuseTransfer(bool state)
    {
        reuse = state;
    }

private:
    struct message*         msg;
    struct message_updater* msg_updater;
    struct message_log*     msg_log;
    std::string             hostname;

#ifdef __STDC_NO_ATOMICS__
    std::atomic<bool> isTerminalSent;
#else
    std::atomic_bool isTerminalSent;
#endif

    bool reuse;
};
