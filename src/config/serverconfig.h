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

#include "config_dev.h"
#include "FileMonitor.h"

#include <map>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>


FTS3_CONFIG_NAMESPACE_START

/* ========================================================================== */

/** \brief Class representing server configuration. Server configuration read once,
 * when the server starts. It provides read-only singleton access. */
class ServerConfig
{
public:
    /// Constructor
    ServerConfig();

    /* ---------------------------------------------------------------------- */

    /// Destructor
    virtual ~ServerConfig();

    /* ---------------------------------------------------------------------- */

    /** Read the configurations */
    void read
    (
        int argc, /**< The command line arguments (from main) */
        char** argv, /**< The command line arguments (from main) */
        bool monitor = false
    );

    /* ---------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
    /** Read the configurations from config file only */
    void read
    (
        const std::string& aFileName /**< Config file name */
    );
#endif // FTS3_COMPILE_WITH_UNITTEST

    /* ---------------------------------------------------------------------- */

    /** General getter of an option. It returns the option, converted to the
     * desired type. Throws exception if the option is not found. */
    template <typename RET> RET get
    (
        const std::string& aVariable /**< A config variable name. */
    );

protected:

    /** Type of the internal store of config variables. */
    typedef std::map<std::string, std::string> _t_vars;

    /* ---------------------------------------------------------------------- */

    /** Return the variable value as a string. */
    const std::string& _get_str
    (
        const std::string& aVariable  /**< A config variable name. */
    );

    /* ---------------------------------------------------------------------- */

    /** Read the configurations - using injected reader type. */
    template <typename READER_TYPE>
    void _read
    (
        int argc, /**< The command line arguments (from main) */
        char** argv /**< The command line arguments (from main) */
    )
    {
        READER_TYPE reader;
        waitIfGetting();
        _vars = reader(argc, argv);
        readTime = time(0);
        notifyGetters();
    }

    /* ---------------------------------------------------------------------- */

    /** Read the configurations from config file only - using injected reader.*/
    template <typename READER_TYPE>
    void _read
    (
        const std::string& aFileName /**< Config file name */
    )
    {
        READER_TYPE reader;
        _vars = reader(aFileName);
    }

    /* ---------------------------------------------------------------------- */

    /** The internal store of config variables. */
    _t_vars _vars;

    /// configuration file monitor
    FileMonitor cfgmonitor;

    ///
    bool reading;
    ///
    int getting;

    ///
    mutex qm;
    ///
    condition_variable qv;

    time_t readTime;

    /**
     *
     */
    void waitIfReading();

    /**
     *
     */
    void notifyReaders();

    /**
     *
     */
    void waitIfGetting();

    /**
     *
     */
    void notifyGetters();

public:
    time_t getReadTime()
    {
        return readTime;
    }
};

/* ========================================================================== */

/** Singleton access to the server config. */
inline ServerConfig& theServerConfig()
{
    static ServerConfig e;
    return e;
}

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
inline std::vector<std::string> ServerConfig::get< std::vector<std::string> > (const std::string& aVariable /**< A config variable name. */)
{

    waitIfReading();
    const std::string& str = _get_str(aVariable);
    notifyReaders();

    boost::char_separator<char> sep(";");
    boost::tokenizer< boost::char_separator<char> > tokens(str, sep);
    boost::tokenizer< boost::char_separator<char> >::iterator it;

    std::vector<std::string> ret;
    for (it = tokens.begin(); it != tokens.end(); ++it)
        {
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

    _t_vars::iterator it;
    for (it = _vars.begin(); it != _vars.end(); ++it)
        {
            if (boost::regex_match(it->first, re))
                {
                    ret[it->first] = it->second;
                }
        }

    notifyReaders();

    return ret;
}

FTS3_CONFIG_NAMESPACE_END

