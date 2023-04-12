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

#include <errno.h>
#include "heuristics.h"
#include "common/Logger.h"
#include <boost/algorithm/string.hpp>

using namespace fts3::common;


static bool findSubstring(const std::string &stack, const char *needles[])
{
    for (size_t i = 0; needles[i] != NULL; ++i) {
        if (stack.find(needles[i]) != std::string::npos)
            return true;
    }
    return false;
}


bool retryTransfer(int errorNo, const std::string &category, const std::string &message)
{
    // If the following strings appear in the error message, assume retriable
    const char *msg_imply_retry[] = {
        "performance marker",
        "Name or service not known",
        "Connection timed out",
        "end-of-file was reached",
        "end of file occurred",
        "SRM_INTERNAL_ERROR",
        "was forcefully killed",
        "operation timeout",
        NULL
    };

    if (findSubstring(message, msg_imply_retry))
        return true;

    // ETIMEDOUT shortcuts the following
    if (errorNo == ETIMEDOUT)
        return true;

    // Do not retry cancelation
    if (errorNo == ECANCELED)
        return false;

    // If the following strings appear in the error message, assume non-retriable
    const char *msg_imply_do_not_retry[] = {
        "proxy expired",
        "with an error 550 File not found",
        "File exists and overwrite",
        "No such file or directory",
        "SRM_INVALID_PATH",
        "The certificate has expired",
        "The available CRL has expired",
        "SRM Authentication failed",
        "SRM_DUPLICATION_ERROR",
        "SRM_AUTHENTICATION_FAILURE",
        "SRM_NO_FREE_SPACE",
        "digest too big for rsa key",
        "Can not determine address of local host",
        "Permission denied",
        "System error in write into HDFS",
        "File exists",
        "checksum do not match",
        "CHECKSUM MISMATCH",
        "The source file is not ONLINE",
        NULL
    };

    if (findSubstring(message, msg_imply_do_not_retry))
        return false;

    // Rely on the error codes otherwise
    if (category == "SOURCE") {
        switch (errorNo) {
            case ENOENT:          // No such file or directory
            case EPERM:           // Operation not permitted
            case EACCES:          // Permission denied
            case EISDIR:          // Is a directory
            case ENAMETOOLONG:    // File name too long
            case E2BIG:           // Argument list too long
            case ENOTDIR:         // Part of the path is not a directory
            case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
                return false;
        }
    }
    else if (category == "DESTINATION") {
        switch (errorNo) {
            case EPERM:           // Operation not permitted
            case EACCES:          // Permission denied
            case EISDIR:          // Is a directory
            case ENAMETOOLONG:    //  File name too long
            case E2BIG:           //  Argument list too long
            case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
            case EEXIST:          // Destination file exists
                return false;
        }
    }
    else //TRANSFER
    {
        switch (errorNo) {
            case ENOSPC:          // No space left on device
            case EPERM:           // Operation not permitted
            case EACCES:          // Permission denied
            case EEXIST:          // File exists
            case EFBIG:           // File too big
            case EROFS:           // Read-only file system
            case ENAMETOOLONG:    // File name too long
            case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
                return false;
        }
    }

    return true;
}


unsigned adjustTimeoutBasedOnSize(uint64_t sizeInBytes, const unsigned addSecPerMb)
{
    static const unsigned long MB = 1 << 20;

    // Reasonable time to wait per MB transferred (default 2 seconds / MB)
    unsigned timeoutPerMBLocal = (addSecPerMb > 0) ? addSecPerMb : 2;

    // Final timeout adjusted considering transfer timeout
    return static_cast<unsigned int>(600 + ceil(timeoutPerMBLocal * (static_cast<double>(sizeInBytes) / MB)));
}


std::string mapErrnoToString(int err)
{
    char buf[256] = {0};
    char const *str = strerror_r(err, buf, sizeof(buf));
    if (str) {
        std::string rep(str);
        std::replace(rep.begin(), rep.end(), ' ', '_');
        return boost::to_upper_copy(rep);
    }
    return "GENERAL ERROR";
}


std::string replaceMetadataString(const std::string &text)
{
    std::string copy = boost::replace_all_copy(text, "?"," ");
    copy = boost::replace_all_copy(copy, "\\\"","\"");
    return copy;
}
