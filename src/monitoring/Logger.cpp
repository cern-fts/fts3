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
#include "utility_routines.h"
#include "Logger.h"
#include <vector>



/**
 * This is the actual workhorse of the logging system.
 */

void logger::writeLog(const std::string& message, bool console)
{
    static int nb_commits = 0;

    if (console == true && message.length() > 0)
        {
            std::string timestapStr = timestamp();
            timestapStr.erase(timestapStr.end() - 1);
            std::cerr << "MSG_ERROR " <<  timestapStr << " " <<  message << std::endl;

            ++nb_commits;
            if (nb_commits > 1000)
                {
                    nb_commits = 0;
                    std::cerr.clear();
                }
        }

    writeMsg(message);
}


/**
 * if logging messages to a file is enabled, append the message content(one line per message)
 */

void logger::writeMsg(const std::string& message)
{

    std::string timestapStr = timestamp();
    timestapStr.erase(timestapStr.end() - 1);
    std::string msg =  timestapStr + " " + message;
    bool isStartedMsg = (message.compare(0, 2, "ST")) == 0? true: false;
    bool isCompletedMsg = (message.compare(0, 2, "CO")) == 0? true: false;


    if(getENABLEMSGLOG() == true)
        {
            if(isStartedMsg == true || isCompletedMsg == true)
                {
                    appendMessageToLogFile(msg);
                }
        }

    if(getENABLELOG() == true)
        {
            if(isStartedMsg == false && isCompletedMsg == false)
                {
                    appendMessageToLogFile(msg);
                }
        }
}


void logger::writeMsgNoConfig(const std::string& message)
{
    std::string timestapStr = timestamp();
    timestapStr.erase(timestapStr.end() - 1);
    std::string msg =  timestapStr + " " + message;
    appendMessageToLogFileNoConfig(msg);
}


void logger::writeError(const char* file, const char* func, const std::string& message)
{
    std::ostringstream full_msg;
    full_msg << "MSG_ERROR In " << file << ":" << func << ": " << message;
    logger::writeLog(full_msg.str(), true);
}
