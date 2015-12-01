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
#ifndef DAEMONTOOLS_H_
#define DAEMONTOOLS_H_

#include <sys/types.h>
#include <string>

namespace fts3 {
namespace common {

/// Returns the user UID for the given name
/// Throws exception if not found, or on error
uid_t getUserUid(const std::string& name);

/// Returns the group GID for the given name
/// Throws exception if not found, or on error
gid_t getGroupGid(const std::string& name);

/// Returns how many processes with the given name are running
/// @return < 0 on error, number of processes with the given name otherwise
int countProcessesWithName(const std::string& name);

/// Checks if there is a binary 'name' in the PATH, and it is executable
/// @param fullPath Stores here the full path, if found.
bool binaryExists(const std::string& name, std::string* fullPath);

/// Change uid, gid, euid and egid
/// Throws SystemError on error
/// @param user  User name
/// @param group Group name
/// @return true if the uid and gid were changed. false if already running as unprivileged (non root)
bool dropPrivileges(const std::string& user, const std::string& group);

} // namespace common
} // namespace fts3

#endif // DAEMONTOOLS_H_
