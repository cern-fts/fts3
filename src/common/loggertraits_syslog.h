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

/** \file loggertraits_syslog.h Interface of LoggerTraits_Syslog. */

#pragma once

#include "dev.h"
#include <syslog.h>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <string>

namespace fts3 {
namespace common {

#undef DEBUG
#define FTS3_COMMON_LOGGER_SUPPORTED_LOG_LEVELS (EMERG)(DEBUG)(WARNING)(INFO)(ALERT)(CRIT)(ERR)(NOTICE)

/** \brief Traits for logger, using syslog. */
struct LoggerTraits_Syslog
{
    /*#define FTS3_TMP_ACTION(r,data,t) t = BOOST_PP_CAT(LOG_, t),

    enum {
        BOOST_PP_SEQ_FOR_EACH(FTS3_TMP_ACTION, ~, FTS3_COMMON_LOGGER_SUPPORTED_LOG_LEVELS)
        UNDEFINED
    };

    #undef FTS3_TMP_ACTION
    */
    enum {EMERG,DEBUG,WARNING,INFO,ALERT,CRIT,ERR,NOTICE};

    /// Open syslog file
    static void openLog();

    /* ---------------------------------------------------------------------- */

    /// Close syslog file
    static void closeLog();

    /* ---------------------------------------------------------------------- */

    /// Write to syslog file
    static void sysLog(const int aLogLevel, const char* aMessage);

    /* ---------------------------------------------------------------------- */

    /// Write the actual system error
    static const char* strerror(const int aErrno);

    /* ---------------------------------------------------------------------- */

    /// Initial static conten of the log line, when starting a new log
    static const std::string initialLogLine();
};

} // end namespace common
} // end namespace fts3

