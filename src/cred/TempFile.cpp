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

#include "TempFile.h"

#include <string.h>
#include <sys/stat.h>
#include "common/Exceptions.h"
#include "common/Logger.h"


TempFile::TempFile(const std::string& prefix, const std::string& dir)
{
    if (prefix.empty()) {
        throw fts3::common::SystemError("Empty prefix");
    }

    char tmp_proxy[FILENAME_MAX];
    if (!dir.empty()) {
        snprintf(tmp_proxy, FILENAME_MAX, "%s/%s.XXXXXX", dir.c_str(),
                prefix.c_str());
    }
    else {
        snprintf(tmp_proxy, FILENAME_MAX, "%s.XXXXXX", prefix.c_str());
    }

    
    int fd = mkstemp(tmp_proxy);
    
    if (fd == -1) {
        std::string reason = (std::string) "Cannot create temporary file <"+ tmp_proxy + ">.    Error is: " + strerror(errno);
        throw fts3::common::SystemError(reason);
    }
    fchmod(fd, 0660);
    m_filename = tmp_proxy;
}


TempFile::~TempFile() throw ()
{
    ::unlink(m_filename.c_str());
}


const std::string& TempFile::name() const throw()
{
    return m_filename;
}


void TempFile::rename(const std::string& name)
{
    // Check Preconditions
    if (m_filename.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Empty TempFile name"
                << fts3::common::commit;
    }
    if (name.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Empty destination name"
                << fts3::common::commit;
    }
    // Rename File
    int r = ::rename(m_filename.c_str(), name.c_str());
    if (0 != r) {
        std::string reason =
                (std::string) "Cannot rename temporary file. Error is: "
                        + strerror(errno);
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << reason << fts3::common::commit;
        ::unlink(m_filename.c_str());
    }
    // Release Ownership
    m_filename.clear();
}
