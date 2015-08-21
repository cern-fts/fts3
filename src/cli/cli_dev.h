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

#pragma once

#include "common/dev.h"

#ifndef FTS3_CLI_NAMESPACE_START
/** Start FTS3 cli namespace. Example:
    \code
    FTS3_CLI_NAMESPACE_START
        code....
        ...
    FTS3_CLI_NAMESPACE_END
    \endcode
*/
#define FTS3_CLI_NAMESPACE_START \
    FTS3_NAMESPACE_START \
    namespace cli {
#endif

#ifndef FTS3_CLI_NAMESPACE_END
/** End FTS3 cli namespace. Example:
    \code
    FTS3_CLI_NAMESPACE_START
        code....
        ...
    FTS3_CLI_NAMESPACE_END
    \endcode
*/
#define FTS3_CLI_NAMESPACE_END \
    FTS3_NAMESPACE_END }
#endif

#ifndef FTS3_CLI_NAMESPACE
/** Defines FTS3 cli namespace name. */
#define FTS3_CLI_NAMESPACE fts3::cli
#endif

