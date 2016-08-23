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
#include <boost/thread/mutex.hpp>


namespace fts3 {
namespace common {

class Logger;

// Logger entry
class LoggerEntry {
private:
    friend class Logger;
    friend LoggerEntry& commit(LoggerEntry& entry);

    std::stringstream stream;
    bool writeable;

    LoggerEntry(bool writeable);

    /// Commits (writes) the actual log line.
    void _commit();

public:
    ~LoggerEntry();

    /// Handles stream modifiers as well. So, you can do things like
    ///
    /// \code
    /// ... << message << commit
    /// \endcode
    LoggerEntry& operator << (LoggerEntry& (*aFunc)(LoggerEntry &aLogger));

    /// Write any type to the output streams
    template <typename T>
    LoggerEntry& operator << (const T& aSrc)
    {
        if (writeable)
        {
            stream << aSrc;
        }
        return *this;
    }
};

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

    /// Get the corresponding log level for the given string
    /// representation. Case insensitive. If repr is unknown, a SystemError
    /// exception is thrown
    static LogLevel getLogLevel(const std::string& repr);

    /// Constructor
    Logger();

    /// Destructor
    virtual ~Logger();

    /// Switch logging on. Log messages will be displayed.
    Logger& setLogLevel(LogLevel level);

    /// Start a new log message. But this is not the recommended way,
    /// use FTS3_COMMON_LOGGER_NEWLOG. It calls this method, but adds
    /// proper debug information. Th einteher LOGLEVEL template parameter
    /// is understood by the logger traits.
    LoggerEntry& newLog(LogLevel logLevel, const char* aFile,
            const char* aFunc, const int aLineNo);

    /// Redirect the output and error streams
    /// @param stdout   File for the standard output
    /// @param stderr   File for the standard error
    /// Return 0 on success
    int redirect(const std::string& stdout, const std::string& stderr) throw();

private:
    friend class LoggerEntry;

    /// Log level
    LogLevel _logLevel;

    /// Separator for the logging
    std::string _separator;

    // Where to write
    boost::mutex outMutex;
    std::ostream *ostream;

    /// Check file descriptor every X iterations
    static const unsigned NB_COMMITS_BEFORE_CHECK = 1000;
    unsigned _nCommits;

    void flush(const std::string &line);

    /// String representation of the timestamp
    static std::string timestamp();

    /// String representation of the log level
    static std::string logLevelStringRepresentation(LogLevel loglevel);

    /// When the disk is full, out fail bits are set
    /// Afterwards, any attempt to write will fail even if space is recovered
    /// So we clean here the fail bits to keep doing business as usual
    void checkFd(void);
};


/// Stream modifier: writes actual system error into the log stream.
LoggerEntry& commit(LoggerEntry& entry);


/// Singleton access of logger.
Logger& theLogger();


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
