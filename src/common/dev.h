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

/** \file dev.h Common FTS3 coding constructs. Each FTS3 source files must include it. */

#pragma once

#include <assert.h>

#ifndef FTS3_NAMESPACE_START
/** Start FTS3 namespace. Example:
    \code
    FTS3_NAMESPACE_START
        code....
        ...
    FTS3_NAMESPACE_END
    \endcode
*/
#define FTS3_NAMESPACE_START namespace fts3 {
#endif

#ifndef FTS3_NAMESPACE_END
/** End FTS3 namespace. Example:
    \code
    FTS3_NAMESPACE_START
        code....
        ...
    FTS3_NAMESPACE_END
    \endcode
*/
#define FTS3_NAMESPACE_END }
#endif

#ifndef FTS3_NAMESPACE
/** Defines FTS3 namespace name. */
#define FTS3_NAMESPACE fts3
#endif

/** FTS 3 application label, can be found in different compile-time generated constructs. */
#define FTS3_APPLICATION_LABEL FTS3_Server

/** FTS3 server version */
#define FTS3_SERVER_VERSION "0.0.1"

