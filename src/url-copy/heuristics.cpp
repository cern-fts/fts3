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
#include <limits.h>
#include "common/definitions.h"
#include "heuristics.h"
#include "common/Logger.h"

using namespace fts3::common;


static bool findSubstring(const std::string& stack, const char* needles[])
{
    for (size_t i = 0; needles[i] != NULL; ++i) {
        if (stack.find(needles[i]) != std::string::npos)
            return true;
    }
    return false;
}


static bool retryTransferInternal(int errorNo, const std::string& category, const std::string& message)
{
    // ETIMEDOUT shortcuts the following
    if (errorNo == ETIMEDOUT)
        return true;

    // If the following strings appear in the error message, assume retriable
    const char* msg_imply_retry[] = {
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

    // If the following strings appear in the error message, assume non-retriable
    const char* msg_imply_do_not_retry[] = {
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
        NULL
    };

    if (findSubstring(message, msg_imply_do_not_retry))
        return false;

    // Rely on the error codes otherwise
    if (category == "SOURCE")
        {
            switch (errorNo)
                {
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
    else if (category == "DESTINATION")
        {
            switch (errorNo)
                {
                case EPERM:           // Operation not permitted
                case EACCES:          // Permission denied
                case EISDIR:          // Is a directory
                case ENAMETOOLONG:    //  File name too long
                case E2BIG:           //  Argument list too long
                case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
                    return false;
                }
        }
    else //TRANSFER
        {
            switch (errorNo)
                {
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


bool retryTransfer(int errorNo, const std::string& category, const std::string& message)
{
    bool retry = retryTransferInternal(errorNo, category, message);
    if (retry)
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Recoverable error: [" << errorNo << "] " << message << commit;
    else
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Non-recoverable error: [" << errorNo << "] " << message << commit;
    return retry;
}


unsigned adjustStreamsBasedOnSize(off_t sizeInBytes, unsigned int /*currentStreams*/)
{
    if (sizeInBytes <= 5242880) //starting with 5MB
        return 1;
    else if (sizeInBytes <= 10485760)
        return 2;
    else if (sizeInBytes > 10485760 && sizeInBytes <= 52428800)
        return 3;
    else if (sizeInBytes > 52428800 && sizeInBytes <= 104857600)
        return 4;
    else if (sizeInBytes > 104857600 && sizeInBytes <= 209715200)
        return 5;
    else if (sizeInBytes > 209715200 && sizeInBytes <= 734003200)
        return 6;
    else if (sizeInBytes > 734003200 && sizeInBytes <= 1073741824)
        return 7;
    else if (sizeInBytes > 1073741824 && sizeInBytes <= 1610612736)
        return 8;
    else if (sizeInBytes > 1610612736 && sizeInBytes <= 2010612736)
        return 9;
    else if (sizeInBytes > 2010612736 && sizeInBytes <= 2576980377)
        return 10;
    else if (sizeInBytes > 2576980377 && sizeInBytes <= 3758096384)
        return 12;
    else if (sizeInBytes > 3758096384 && sizeInBytes <= 4858096384)
        return 14;
    else
        return 16;
    return 6;
}



static unsigned adjustTimeout(off_t sizeInBytes)
{
    if (sizeInBytes <= 10485760) //starting with 10MB
        return 900;
    else if (sizeInBytes > 10485760 && sizeInBytes <= 52428800)
        return 1100;
    return 3600;
}



unsigned adjustTimeoutBasedOnSize(off_t sizeInBytes, unsigned timeout, unsigned timeoutPerMB, bool global_timeout)
{
    static const unsigned long MB = 1 << 20;

    // Reasonable time to wait per MB transferred
    // If input timeout is 0, give it a little more room
    double timeoutPerMBLocal = 0;
    if(timeoutPerMB > 0 )
        timeoutPerMBLocal = timeoutPerMB;
    else
        timeoutPerMBLocal = 2;

    // Reasonable time to wait for the transfer itself
    unsigned transferTimeout = 0;
    if(global_timeout)
        transferTimeout = timeout;
    else
        transferTimeout = adjustTimeout(sizeInBytes);

    // Final timeout adjusted considering transfer timeout
    long double totalTimeout = transferTimeout + ceil(timeoutPerMBLocal * (static_cast<double>(sizeInBytes) / MB));

    // Truncate to an unsigned
    if (totalTimeout >= INT_MAX)
        timeout = 4000;
    else
        timeout = static_cast<unsigned>(totalTimeout);

    if(timeout > 30000)
        timeout = 30000;

    return timeout;
}
