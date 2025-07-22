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

/** \file serverconfig.cpp Implementation of FTS3 server configuration. */

#include <boost/program_options.hpp>

#include "common/Exceptions.h"
#include "ServerConfigReader.h"
#include "ServerConfig.h"


using namespace fts3::config;
using namespace fts3::common;

template class fts3::common::Singleton<ServerConfig>;

ServerConfig::ServerConfig() : reading(0), getting(0), readTime(0)
{
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "ServerConfig created" << commit;
}


ServerConfig::~ServerConfig()
{
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "ServerConfig destroyed" << commit;
}


const std::string &ServerConfig::_get_str(const std::string &aVariable)
{
    _t_vars::iterator itr = _vars.find(aVariable);

    if (itr == _vars.end()) {
        throw UserError("Server config variable " + aVariable + " not defined.");
    }

    // No worry, it will not be 0 pointer due to the exception
    return itr->second;
}


void ServerConfig::read(int argc, char** argv)
{
    _read<ServerConfigReader> (argc, argv);
}


time_t ServerConfig::getReadTime()
{
    return readTime;
}


void ServerConfig::waitIfReading()
{
    boost::mutex::scoped_lock lock(qm);
    while (reading) {
        qv.wait(lock);
    }
    getting++;
}


void ServerConfig::notifyReaders()
{
    boost::mutex::scoped_lock lock(qm);
    getting--;
    qv.notify_all(); // there is anyway only one thread to be notified
}


void ServerConfig::waitIfGetting()
{
    boost::mutex::scoped_lock lock(qm);
    while (getting > 0) qv.wait(lock);
    reading = true;
}


void ServerConfig::notifyGetters()
{
    boost::mutex::scoped_lock lock(qm);
    reading = false;
    qv.notify_all();
}
