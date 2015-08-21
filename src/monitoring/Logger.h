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

#ifndef LOGGER_H

#include <iostream>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <iostream>
#include <vector>

using namespace std;

class logger
{
public:
    static void writeLog(const std::string& message, bool console = false);
    static void writeMsg(const std::string& message);
    static void writeMsgNoConfig(const std::string& message);
    static void writeError(const char* file, const char* func, const std::string& message);
};

#define LOGGER_ERROR(message) logger::writeError(__FILE__, __func__, message)

#endif
