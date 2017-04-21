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

/** \file serverconfig.h Define FTS3 server configuration. */

#pragma once
#ifndef SERVERCONFIG_H_
#define SERVERCONFIG_H_

#include "FileMonitor.h"

#include <map>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "common/Singleton.h"


namespace fts3 {
namespace config {

/** \brief Class representing server configuration. Server configuration read once,
 * when the server starts. It provides read-only singleton access. */
class ServerConfig: public fts3::common::Singleton<ServerConfig>
{
public:
    /// Constructor
    ServerConfig();

    /// Destructor
    virtual ~ServerConfig();

    /// Read the configurations
    /// @param argc     The number of command line arguments
    /// @param argv     The command line arguments
    void read(int argc, char** argv);

    /// Start the thread that watches for changes on the configuration file
    /// @note Beware of forks! Remember threads are gone after a fork
    void startMonitor(void);

    /// Read the configurations from config file only
    void read(const std::string& aFileName);

    /// Return the last time the configuration was reloaded
    time_t getReadTime();

    /// General getter of an option. It returns the option, converted to the
    /// desired type. Throws exception if the option is not found.
    template <typename RET> RET get(const std::string& aVariable);

protected:

    /// Type of the internal store of config variables.
    typedef std::map<std::string, std::string> _t_vars;

    /// Return the variable value as a string.
    const std::string& _get_str(const std::string& aVariable);

    /// Read the configurations - using injected reader type.
    template <typename READER_TYPE>
    void _read(int argc, char** argv)
    {
        READER_TYPE reader;
        waitIfGetting();
        _vars = reader(argc, argv);
        readTime = time(0);
        notifyGetters();
    }

    /// Read the configurations from config file only - using injected reader.
    template <typename READER_TYPE>
    void _read(const std::string& aFileName)
    {
        READER_TYPE reader;
        _vars = reader(aFileName);
    }

    /// The internal store of config variables.
    _t_vars _vars;

    /// Configuration file monitor
    FileMonitor cfgmonitor;

    bool reading;
    int getting;

    boost::mutex qm;
    boost::condition_variable qv;

    time_t readTime;

    void waitIfReading();
    void notifyReaders();
    void waitIfGetting();
    void notifyGetters();
};


template <typename RET>
RET ServerConfig::get (const std::string& aVariable /**< A config variable name. */)
{

    waitIfReading();
    const std::string& str = _get_str(aVariable);
    notifyReaders();

    return boost::lexical_cast<RET>(str);
}

template <>
inline bool ServerConfig::get<bool> (const std::string& aVariable /**< A config variable name. */)
{

    waitIfReading();
    std::string str = _get_str(aVariable);
    notifyReaders();

    boost::to_lower(str);

    // if the string is 'false' return false
    if (str == "false") return false;
    // otherwise return true (it may be 'true' or other string containing respective value)
    else return true;
}


template <>
inline boost::posix_time::time_duration ServerConfig::get<boost::posix_time::time_duration> (const std::string& aVariable /**< A config variable name. */)
{
    waitIfReading();
    std::string str = _get_str(aVariable);
    notifyReaders();
    return boost::posix_time::seconds(boost::lexical_cast<int>(str));
}


template <>
inline std::vector<std::string> ServerConfig::get< std::vector<std::string> > (const std::string& aVariable /**< A config variable name. */)
{

    waitIfReading();
    const std::string& str = _get_str(aVariable);
    notifyReaders();

    boost::char_separator<char> sep(";");
    boost::tokenizer< boost::char_separator<char> > tokens(str, sep);

    std::vector<std::string> ret;
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        ret.push_back(*it);
    }

    return ret;
}

template <>
inline std::map<std::string, std::string> ServerConfig::get< std::map<std::string, std::string> > (const std::string& aVariable)
{

    std::map<std::string, std::string> ret;
    boost::regex re(aVariable);

    waitIfReading();

    for (auto it = _vars.begin(); it != _vars.end(); ++it) {
        if (boost::regex_match(it->first, re)) {
            ret[it->first] = it->second;
        }
    }

    notifyReaders();

    return ret;
}

} // end namespace config
} // end namespace fts3

#endif // SERVERCONFIG_H_
