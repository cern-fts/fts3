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
#include "utility_routines.h"
#include <iostream>
#include <string>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "Logger.h"
#include <vector>
#include <boost/filesystem.hpp>
#include "common/definitions.h"
#include <boost/lexical_cast.hpp>

#include "../common/ConcurrentQueue.h"

extern bool stopThreads;
static bool signalReceived = false;

namespace fs = boost::filesystem;

void handler(int)
{
    if(!signalReceived)
        {
            signalReceived = true;

            stopThreads = true;
            std::queue<std::string> myQueue = ConcurrentQueue::getInstance()->the_queue;
            std::string ret;
            while(!myQueue.empty())
                {
                    ret = myQueue.front();
                    myQueue.pop();
                    restoreMessageToDisk(ret);
                }
            sleep(5);
            exit(0);
        }
}

MsgPipe::MsgPipe()
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
    std::vector<struct message_monitoring> messages;
    std::vector<struct message_monitoring>::const_iterator iter;

    while (stopThreads==false)
        {
            try
                {

                    if(fs::is_empty(fs::path(MONITORING_DIR)))
                        {
                            sleep(1);
                            continue;
                        }

                    int returnValue = runConsumerMonitoring(messages);
                    if(returnValue != 0)
                    {
                        std::ostringstream errorMessage;
                        errorMessage << "runConsumerMonitoring returned " << returnValue;
                        LOGGER_ERROR(errorMessage.str());
                    }

                    if(!messages.empty())
                        {
                            for (iter = messages.begin(); iter != messages.end(); ++iter)
                                {
                                    ConcurrentQueue::getInstance()->push( (*iter).msg );
                                }
                            messages.clear();
                        }
                    sleep(1);
                }
            catch (const fs::filesystem_error& ex)
                {
                    LOGGER_ERROR(ex.what());
                    cleanup();
                    sleep(1);
                }
            catch (...)
                {
                    LOGGER_ERROR("Unexpected exception");
                    cleanup();
                    sleep(1);
                }
        }
}

void MsgPipe::cleanup()
{
    std::queue<std::string> myQueue = ConcurrentQueue::getInstance()->the_queue;
    std::string ret;
    while(!myQueue.empty())
        {
            ret = myQueue.front();
            myQueue.pop();
            restoreMessageToDisk(ret);
        }
}
