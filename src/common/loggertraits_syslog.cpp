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

/** \file loggertraits_syslog.cpp Implementation of LoggerTraits_Syslog class. */

#include "loggertraits_syslog.h"
#include <string.h>
#include <boost/preprocessor/stringize.hpp>

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "unittest/testsuite.h"
#include <errno.h>
#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ---------------------------------------------------------------------- */

#define FTS3_COMMON_LOGGER_FACILITY LOG_USER
#define FTS3_COMMON_LOGGER_OPTION LOG_PID | LOG_PERROR | LOG_CONS

using namespace fts3::common;

/* ========================================================================== */

void LoggerTraits_Syslog::openLog()
{
    ::openlog
    (
        BOOST_PP_STRINGIZE(FTS3_APPLICATION_LABEL),
        FTS3_COMMON_LOGGER_OPTION,
        FTS3_COMMON_LOGGER_FACILITY
    );
}

/* ---------------------------------------------------------------------- */

void LoggerTraits_Syslog::closeLog()
{
    ::closelog();
}

/* ---------------------------------------------------------------------- */

void LoggerTraits_Syslog::sysLog(const int aLogLevel, const char* aMessage)
{
    ::syslog (aLogLevel, "%s", aMessage);
}

/* ---------------------------------------------------------------------- */

const char* LoggerTraits_Syslog::strerror(const int aErrno)
{
    return ::strerror (aErrno);
}

/* ---------------------------------------------------------------------- */

const std::string LoggerTraits_Syslog::initialLogLine()
{
    return std::string();
}

/* ========================================================================== */

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

BOOST_AUTO_TEST_SUITE( common )
BOOST_AUTO_TEST_SUITE(SyslogTest)

BOOST_FIXTURE_TEST_CASE (Common__LoggerTraits_SysLog, LoggerTraits_Syslog)
{
    BOOST_TEST_MESSAGE ("\n**** This is a visual test. Ensure that the messages below"
                        "are visible (or not visible) in /var/log/syslog. TODO: automate it.\n");

    openLog();
    sysLog(LOG_DEBUG, "MESSAGE MUST BE PRESENT");
    closeLog();
    sysLog(LOG_DEBUG, "MESSAGE MUST NOT BE PRESENT (OR DISPLAYED DIFFERENTLY)");

    std::string errmsg(strerror (EACCES));
    BOOST_CHECK_EQUAL (errmsg, "Permission denied");
    BOOST_TEST_MESSAGE ("\n**** End of manual test.");
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS
