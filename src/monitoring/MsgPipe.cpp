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

#include "MsgPipe.h"
#include <iostream>
#include <string>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "common/Logger.h"
#include "common/ConcurrentQueue.h"
#include "UtilityRoutines.h"

extern bool stopThreads;
static bool signalReceived = false;

using fts3::common::ConcurrentQueue;
namespace fs = boost::filesystem;

void handler(int)
{
    if (!signalReceived) {
        signalReceived = true;

        stopThreads = true;
        std::queue<std::string> myQueue = ConcurrentQueue::getInstance()->theQueue;
        std::string ret;
        while (!myQueue.empty()) {
            ret = myQueue.front();
            myQueue.pop();
            //restoreMessageToDisk(ret);
        }
        sleep(5);
        exit(0);
    }
}


MsgPipe::MsgPipe(const std::string &baseDir):
    consumer(baseDir), producer(baseDir)
{
    //register sig handler to cleanup resources upon exiting
    signal(SIGFPE, handler);
    signal(SIGILL, handler);
    signal(SIGSEGV, handler);
    signal(SIGBUS, handler);
    signal(SIGABRT, handler);
    signal(SIGTERM, handler);
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);
}


MsgPipe::~MsgPipe()
{
}


void MsgPipe::run()
{
    std::vector<std::string> messages;

    while (stopThreads == false) {
        try {
            int returnValue = consumer.runConsumerMonitoring(messages);
            if (returnValue != 0) {
                std::ostringstream errorMessage;
                errorMessage << "runConsumerMonitoring returned " << returnValue;
                FTS3_COMMON_LOGGER_LOG(ERR, errorMessage.str());
            }

            for (auto iter = messages.begin(); iter != messages.end(); ++iter) {
                ConcurrentQueue::getInstance()->push(*iter);
            }
            messages.clear();
        }
        catch (const fs::filesystem_error &ex) {
            FTS3_COMMON_LOGGER_LOG(ERR, ex.what());
            cleanup();
        }
        catch (...) {
            FTS3_COMMON_LOGGER_LOG(CRIT, "Unexpected exception");
            cleanup();
        }
        sleep(1);
    }
}


void MsgPipe::cleanup()
{
    std::queue<std::string> myQueue = ConcurrentQueue::getInstance()->theQueue;
    std::string ret;
    while (!myQueue.empty()) {
        ret = myQueue.front();
        myQueue.pop();
        restoreMessageToDisk(producer, ret);
    }
}
