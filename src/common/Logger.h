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

/** \file Logger.h Interface of FTS3 logger component. */

#pragma once
#ifndef LOGGER_H_
#define LOGGER_H_

#include <iostream>

namespace fts3 {
namespace common {

/// Logger class
class Logger
{
public:

    typedef enum
    {
        TRACE,
        DEBUG,
        INFO,
        NOTICE,
        WARNING,
        ERR,
        CRIT
    } LogLevel;

    /// Constructor
    Logger();

    /// Destructor
    virtual ~Logger ();

    /// Switch logging on. Log messages will be displayed.
    Logger& setLogOn();

    /// Switch log messages off. No log messages are displayed.
    Logger& setLogOff();

    /// Start a new log message. But this is not the recommended way,
    /// use FTS3_COMMON_LOGGER_NEWLOG. It calls this method, but adds
    /// proper debug information. Th einteher LOGLEVEL template parameter
    /// is understood by the logger traits.
    Logger& newLog(LogLevel logLevel, const char* aFile,
            const char* aFunc, const int aLineNo);

    /// Handles stream modifiers as well. So, you can do things like
    ///
    /// \code
    /// ... << message << commit
    /// \endcode
    Logger& operator << (Logger& (*aFunc)(Logger &aLogger));

    /// Write any type to the output streams
    template <typename T>
    Logger& operator << (const T& aSrc)
    {
        if (_isLogOn)
        {
            std::cout << aSrc;
        }
        return *this;
    }

    /// Redirect the output and error streams
    /// @param stdout   File for the standard output
    /// @param stderr   File for the standard error
    /// Return 0 on success
    int redirect(const std::string& stdout, const std::string& stderr) throw();

private:
    /// true: log lines are written, false: no log is written.
    bool _isLogOn;

    /// Actual log level
    LogLevel _actLogLevel;

    /// Separator for the logging
    std::string _separator;

    /// Check file descriptor every X iterations
    static const unsigned NB_COMMITS_BEFORE_CHECK = 1000;
    unsigned _nCommits;

    /// String representation of the timestamp
    static std::string timestamp();

    /// String representation of the log level
    static std::string logLevelStringRepresentation(LogLevel loglevel);

    /// Commits (writes) the actual log line.
    void _commit();

    /// When the disk is full, std::cout fail bits are set
    /// Afterwards, any attempt to write will fail even if space is recovered
    /// So we clean here the fail bits to keep doing business as usual
    void checkFd(void);

    // Need to give it access to _commit
    friend Logger& commit(Logger& aLogger);
};


/// Stream modifier: writes actual system error into the log stream.
Logger& commit(Logger& aLogger);


/// Singleton access of logger.
inline Logger& theLogger()
{
    static Logger logger;
    return logger;
}


/// This is how you should use FTS3 logging. For example:
///
/// \code
/// FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Number: " << 3 << commit;
/// \endcode
///
/// You can use normal stream constructs. Do not forget commit!
///
/// The log level labels are system specific,
///
#define FTS3_COMMON_LOGGER_NEWLOG(aLevel)   \
    fts3::common::theLogger().newLog(fts3::common::Logger::aLevel, __FILE__, __FUNCTION__, __LINE__)

/// Log a simple string message in one line (with implicit commit). Parameters:
/// log level label and the message string.
#define FTS3_COMMON_LOGGER_LOG(level,message) \
    FTS3_COMMON_LOGGER_NEWLOG(level) << message << fts3::common::commit


} // namespace common
} // namespace fts3

#endif // LOGGER_H_
