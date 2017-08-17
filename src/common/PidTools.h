/*
 * Copyright (c) CERN 2016
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

#ifndef FTS3_PIDTOOLS_H
#define FTS3_PIDTOOLS_H

#include <string>
#include <cstdint>

namespace fts3 {
namespace common {

/// Get the start time of the given PID, with a resolution of milliseconds
/// On error, it will return 0
uint64_t getPidStartime(pid_t);

// Creates a PID file
// Returns the full path
std::string createPidFile(const std::string &dir, const std::string &name);

}
}

#endif //FTS3_PIDTOOLS_H
