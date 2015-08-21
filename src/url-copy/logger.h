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
#ifndef _LOGGER
#define _LOGGER

#include <cerrno>
#include <fstream>
#include <iostream>
#include <boost/thread/recursive_mutex.hpp>

class Logger
{
public:
    static Logger& getInstance();

    std::ostream& INFO();
    std::ostream& WARNING();
    std::ostream& ERROR();
    std::ostream& DEBUG();

    int redirectTo(const std::string& path, bool debug);

private:
    static Logger instance;

    Logger();
    Logger(const Logger&); // Should never be called
    ~Logger();

    std::ostream *log;
    std::ofstream logHandle;
    boost::recursive_mutex mutex;
};

#endif
