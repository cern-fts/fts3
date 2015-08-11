/********************************************
 * Copyright @ Members of the EMI Collaboration, 2013.
 * See www.eu-emi.eu for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***********************************************/

#pragma once

#include "Profiler.h"

// Open a profiling scope, capturing exceptions
#define PROFILE_START(prefix) \
    fts3::ScopeProfiler __profiler(std::string(prefix) + __func__);\
    try {

// Ends a profiling scope, capturing exceptions
#define PROFILE_END \
    }\
    catch (...) {\
        __profiler.exception();\
        throw;\
    }

// Handy macro to avoid repeating PROFILE_START/PROFILE_END for small functions
#define PROFILE_PREFIXED(prefix, body) \
PROFILE_START(prefix);\
body;\
PROFILE_END;

// Create a ScopeProfiler ONLY if profiling is set
#define PROFILE_SCOPE(scope) \
std::unique_ptr<fts3::ScopeProfiler> __profiler;\
if (fts3::ProfilingSubsystem::getInstance().getInterval() > 0) {\
    __profiler.reset(new fts3::ScopeProfiler(scope));\
}
