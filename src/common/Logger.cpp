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
#include <fstream>
#include <boost/algorithm/string.hpp>


namespace fts3 {
namespace common {


Logger& theLogger()
{
    static Logger *logger = new Logger();
    return *logger;
}

LoggerEntry::LoggerEntry(bool writeable): writeable(writeable)
{
}


LoggerEntry::LoggerEntry(const LoggerEntry& le): stream(le.stream.str()), writeable(le.writeable)
{
}


LoggerEntry::~LoggerEntry()
{
}


LoggerEntry& LoggerEntry::operator << (LoggerEntry& (*aFunc)(LoggerEntry &aLogger))
{
    return aFunc(*this);
}


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


Logger::Logger(): _logLevel(DEBUG), _separator("; "), _nCommits(0)
{
    ostream = &std::cout;
    newLog(TRACE, __FILE__, __FUNCTION__, __LINE__) << "Logger created" << commit;
}


Logger::~Logger ()
{
    newLog(TRACE, __FILE__, __FUNCTION__, __LINE__) << "Logger about to be destroyed" << commit;
}


Logger & Logger::setLogLevel(LogLevel level)
{
    newLog(INFO, __FILE__, __FUNCTION__, __LINE__)
        << "Setting debug level to " << logLevelStringRepresentation(level)
        << commit;
    _logLevel = level;
    return *this;
}


void Logger::flush(const std::string &line)
{
    boost::mutex::scoped_lock lock(outMutex);
    _nCommits++;
    if (_nCommits >= NB_COMMITS_BEFORE_CHECK) {
        _nCommits = 0;
        checkFd();
    }
    *ostream << line << std::endl;
}


/// This method has to be thread safe!
void LoggerEntry::_commit()
{
    if (writeable) {
        std::string line = stream.str();
        theLogger().flush(line);
    }
}


LoggerEntry Logger::newLog(LogLevel level, const char* aFile,
        const char* aFunc, const int aLineNo)
{
    LoggerEntry entry(level >= this->_logLevel);
    entry << logLevelStringRepresentation(level) << timestamp() << _separator;
    if (level >= ERR && this->_logLevel <= DEBUG) {
        entry << aFile << _separator << aFunc << _separator << std::dec << aLineNo << _separator;
    }
    return entry;
}


static int createAndReopen(const std::string& path, FILE* stream)
{
    int fd = ::open(path.c_str(), O_APPEND | O_CREAT, 0644);
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
    if (ostream != &std::cout) {
        delete ostream;
    }
    ostream = new std::ofstream(outPath, std::ios_base::app);

    if (!errPath.empty()) {
        if (createAndReopen(errPath, stderr) < 0)
            return -1;
    }
    return 0;
}


void Logger::checkFd(void)
{
    if (ostream->fail()) {
        ostream->clear();
    }
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


LoggerEntry& commit(LoggerEntry& entry)
{
    entry._commit();
    return entry;
}

} // namespace common
} // namespace fts3
