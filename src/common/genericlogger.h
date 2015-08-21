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

/** \file logger.h Interface of FTS3 logger component. */

#pragma once

#include "monitorobject.h"
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>


namespace fts3 {
namespace common {

/* ========================================================================== */

/** \brief Base of each Logger-s for FTS3 (common logic). */
class LoggerBase
{
protected:
    /// Constructor.
    LoggerBase();
    //mutable ThreadTraits::MUTEX_R _mutex;

    /* ---------------------------------------------------------------------- */

    /// Destructor.
    virtual ~LoggerBase();

    /* ---------------------------------------------------------------------- */

    /// Separator for message parts. In case you want to filter by column.
    static const std::string& _separator();

    /* ====================================================================== */

    /// true: log lines are written, false: no log is written.
    bool _isLogOn;
};

/* ========================================================================== */

/** \file Generic Logger class. Supports dependency injection, etc.
 * You can redefine the behaviour of Logger by using different Traits class.
 * Good for testing as well, to inject dependencies. */
template <typename Traits>
class GenericLogger : public LoggerBase
{
public:
    /// The logger tratis type
    typedef Traits type_traits;


    /* ---------------------------------------------------------------------- */

    /// Constructor
    GenericLogger () :
        LoggerBase(),
        _actLogLevel (static_cast<int>(Traits::INFO)),
        nb_commits(0)
    {
        (*this) << Traits::initialLogLine();
        _commit();
    }

    static std::string timestamp()
    {
        std::string timestampStr("");
        char timebuf[128] = "";
        // Get Current Time
        time_t current;
        time(&current);
        struct tm local_tm;
        localtime_r(&current, &local_tm);
        strftime(timebuf, sizeof(timebuf), "%a %b %d %H:%M:%S %Y", &local_tm);
        timestampStr = timebuf;
        return timestampStr + " ";
    }

    static std::string logLevelStringRepresentation(int loglevel)
    {
        switch (loglevel)
            {
            case 0:
                return std::string("EMERG   ");
            case 1:
                return std::string("DEBUG   ");
            case 2:
                return std::string("WARNING ");
            case 3:
                return std::string("INFO    ");
            case 4:
                return std::string("ALERT   ");
            case 5:
                return std::string("CRIT    ");
            case 6:
                return std::string("ERR     ");
            case 7:
                return std::string("NOTICE  ");
            default:
                return std::string("INFO    ");

            }
    }


    /* ---------------------------------------------------------------------- */

    /// Destructor
    virtual ~GenericLogger ()
    {

    }

    /* ---------------------------------------------------------------------- */

    /// Switch logging on. Log messages will be displayed.
    GenericLogger& setLogOn()
    {
        _isLogOn = true;
        return *this;
    }

    /* ---------------------------------------------------------------------- */

    /// Switch log messages off. No log messages are displayed.
    GenericLogger& setLogOff()
    {
        _isLogOn = false;
        return *this;
    }

    /* ---------------------------------------------------------------------- */

    /// When the disk is full, std::cerr fail bits are set
    /// Afterwards, any attempt to write will fail even if space is recovered
    /// So we clean here the fail bits to keep doing business as usual
    void check_fd(void)
    {
        if (std::cerr.fail())
            {
                std::cerr.clear();
                *this << logLevelStringRepresentation(Traits::WARNING) << timestamp() << _separator();
                *this << "std::cerr fail bit cleared";
            }
        else
            {
                *this << logLevelStringRepresentation(Traits::INFO) << timestamp() << _separator();
                *this << "std::cerr clear!";
            }

        std::cerr << std::endl;
        std::cout << std::endl;
    }


    /* ---------------------------------------------------------------------- */

    /// Commits (writes) the actual log line.
    void _commit()
    {
        std::cout << std::endl;
        std::cerr << std::endl;
        ++nb_commits;
        if (nb_commits >= NB_COMMITS_BEFORE_CHECK)
            {
                nb_commits = 0;
                check_fd();
            }
    }

    /* ---------------------------------------------------------------------- */

    /// Start a new log message. But this is not the recommended way,
    /// use FTS3_COMMON_LOGGER_NEWLOG. It calls this method, but adds
    /// proper debug information. Th einteher LOGLEVEL template parameter
    /// is understood by the logger traits.
    template <int LOGLEVEL>
    GenericLogger& newLog
    (
        const char* aFile, /**< Actual source file */
        const char* aFunc, /**< Actual function that is logging */
        const int aLineNo /**< Line number where the log file was written. */
    )
    {
        _actLogLevel = LOGLEVEL;
        (*this) << logLevelStringRepresentation(_actLogLevel) << timestamp() << _separator();

        bool isDebug = (Traits::ERR == _actLogLevel);
        if (isDebug)
            {
                (*this) << aFile << _separator() << aFunc << _separator() << std::dec << aLineNo << _separator();
            }

        return *this;
    }

    /* ---------------------------------------------------------------------- */

    /// Handles stream modifiers as well. So, you can do things like
    ///
    /// \code
    /// ... << message << commit()
    /// \endcode
    GenericLogger& operator <<
    (
        GenericLogger& (*aFunc)(GenericLogger &aLogger) /**< The functor name */
    )
    {
        return (aFunc)(*this);
    }

    /* ---------------------------------------------------------------------- */
    template <typename T>
    GenericLogger& operator << (const T& aSrc)
    {
        if (_isLogOn)
            {
                std::cout << aSrc;
                std::cerr << aSrc;
            }
        return *this;
    }

    /* ---------------------------------------------------------------------- */

    int open(const std::string& path) throw()
    {
        int filedesc = ::open(path.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (filedesc != -1)   //if ok
            {
                close(filedesc);
                FILE* freopenLogFile = freopen(path.c_str(), "a", stderr);
                if (freopenLogFile == NULL)
                    return -1;
            }
        else
            {
                return -1;
            }
        logPath = path;
        return 0;
    }

    /* ---------------------------------------------------------------------- */

    /// Actual log level (see syslog).
    int _actLogLevel;

    /// Remember the path!
    std::string logPath;

    /// Check file descriptor every X iterations
    static const unsigned NB_COMMITS_BEFORE_CHECK = 1000;
    unsigned nb_commits;
};

/// Stream modifier: writes actual system error into the log stream.
template <typename Traits>
GenericLogger<Traits> & commit
(
    GenericLogger<Traits>& aLogger /**< The logger object to be modified. */
)
{
    aLogger._commit();
    return aLogger;
}

/* -------------------------------------------------------------------------- */

/// Stream modifier: writes actual system error into the log stream.
template <typename Traits>
GenericLogger<Traits> & addErr
(
    GenericLogger<Traits>& aLogger /**< The logger object to be modified. */
)
{
    aLogger << Traits::strerror(errno);
    return aLogger;
}

} // end namespace common
} // end namespace fts3

