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

#include "common_dev.h"
#include "genericlogger.h"

// This macro should define the supported log levels as list of labels!
// Example: #define FTS3_COMMON_LOGGER_SUPPORTED_LOG_LEVELS (EMERG)(DEBUG)
#include "loggertraits_syslog.h"

#ifndef FTS3_COMMON_LOGGER_SUPPORTED_LOG_LEVELS
#error FTS3_COMMON_LOGGER_SUPPORTED_LOG_LEVELS is not defined!
#endif

FTS3_COMMON_NAMESPACE_START

/* -------------------------------------------------------------------------- */

// Use syslogger system-wide.
#include "loggertraits_syslog.h"
typedef GenericLogger<LoggerTraits_Syslog> Logger;

/* -------------------------------------------------------------------------- */

/// Singleton access of logger.
inline Logger& theLogger()
{
    static Logger logger;
    return logger;
}

FTS3_COMMON_NAMESPACE_END

/** This is how you should use FTS3 logging. For example:
 *
 * \code
 * FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Number: " << 3 << commit;
 * \endcode
 *
 * You can use normal stream constructs. Do not forget commit!
 *
 * The log level labels are system specific,
 */
#define FTS3_COMMON_LOGGER_NEWLOG(aLevel)	\
	FTS3_COMMON_NAMESPACE::theLogger().newLog<FTS3_COMMON_NAMESPACE::Logger::type_traits::aLevel>(__FILE__, __FUNCTION__, __LINE__)

/* -------------------------------------------------------------------------- */

/** Log a simple string message in one line (with implicit commit). Parameters:
 log level label and the message string. */
#define FTS3_COMMON_LOGGER_LOG(level,message) \
    FTS3_COMMON_LOGGER_NEWLOG(level) << message << commit

