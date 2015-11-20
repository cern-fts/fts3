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

/** \file Logger.cpp Implementation/tests of Logger class. */

#include "Logger.h"
#include "Exceptions.h"

#include <fcntl.h>
#include <unistd.h>
#include <boost/algorithm/string.hpp>


namespace fts3 {
namespace common {


Logger::LogLevel Logger::getLogLevel(const std::string& repr)
{
    struct LevelRepr {
        const char *repr;
        LogLevel level;
    } ;
    static const LevelRepr LEVEL_REPR[] = {
        {"trace", TRACE}, {"debug", DEBUG},
        {"info",  INFO}, {"notice", NOTICE},
        {"warn", WARNING}, {"warning", WARNING},
        {"err", ERR}, {"error", ERR},
        {"crit", CRIT}, {"critical", CRIT}
    };
    static const int N_REPR = sizeof(LEVEL_REPR) / sizeof(LevelRepr);

    for (int i = 0; i < N_REPR; ++i) {
        if (boost::iequals(repr, LEVEL_REPR[i].repr)) {
            return LEVEL_REPR[i].level;
        }
    }

    throw SystemError(std::string("Unknown logging level ") + repr);
}


Logger::Logger(): _logLevel(DEBUG), _lastLogLevel(DEBUG), _separator("; "), _nCommits(0)
{
}


Logger::~Logger ()
{
}


Logger & Logger::setLogLevel(LogLevel level)
{
    newLog(INFO, __FILE__, __FUNCTION__, __LINE__)
        << "Setting debug level to " << logLevelStringRepresentation(level)
        << commit;
    _logLevel = level;
    return *this;
}


void Logger::_commit()
{
    if (_lastLogLevel >= _logLevel) {
        std::cout << std::endl;
        ++_nCommits;
        if (_nCommits >= NB_COMMITS_BEFORE_CHECK) {
            _nCommits = 0;
            checkFd();
        }
    }
}


Logger& Logger::newLog(LogLevel level, const char* aFile,
        const char* aFunc, const int aLineNo)
{
    _lastLogLevel = level;

    if (level >= this->_logLevel) {
        (*this) << logLevelStringRepresentation(level) << timestamp() << _separator;

        if (level >= ERR && this->_logLevel <= DEBUG) {
            (*this) << aFile << _separator << aFunc << _separator << std::dec << aLineNo << _separator;
        }
    }

    return *this;
}


Logger& Logger::operator << (Logger& (*aFunc)(Logger &aLogger))
{
    return (aFunc)(*this);
}


static int createAndReopen(const std::string& path, FILE* stream)
{
    int fd = ::creat(path.c_str(), 0644);
    if (fd < 0)
        return -1;
    close(fd);
    stream = freopen(path.c_str(), "a", stream);
    if (!stream)
        return -1;
    return 0;
}


int Logger::redirect(const std::string& outPath, const std::string& errPath) throw()
{
    if (createAndReopen(outPath, stdout) < 0)
        return -1;
    if (createAndReopen(errPath, stderr) < 0)
        return -1;
    return 0;
}


void Logger::checkFd(void)
{
    if (std::cout.fail()) {
        std::cout.clear();
        *this << logLevelStringRepresentation(WARNING) << timestamp() << _separator;
        *this << "std::cout fail bit cleared";
    }
    else {
        *this << logLevelStringRepresentation(INFO) << timestamp() << _separator;
        *this << "std::cout clear!";
    }

    std::cout << std::endl;
}


std::string Logger::timestamp()
{
    char timebuf[128] = "";
    // Get Current Time
    time_t current;
    time(&current);
    struct tm local_tm;
    localtime_r(&current, &local_tm);
    strftime(timebuf, sizeof(timebuf), "%a %b %d %H:%M:%S %Y", &local_tm);
    return timebuf;
}


std::string Logger::logLevelStringRepresentation(LogLevel loglevel)
{
    switch (loglevel) {
        case TRACE:
            return std::string("TRACE   ");
        case DEBUG:
            return std::string("DEBUG   ");
        case WARNING:
            return std::string("WARNING ");
        case INFO:
            return std::string("INFO    ");
        case CRIT:
            return std::string("CRIT    ");
        case ERR:
            return std::string("ERR     ");
        case NOTICE:
            return std::string("NOTICE  ");
        default:
            return std::string("INFO    ");
    }
}


Logger& commit(Logger& aLogger)
{
    aLogger._commit();
    return aLogger;
}

} // namespace common
} // namespace fts3
