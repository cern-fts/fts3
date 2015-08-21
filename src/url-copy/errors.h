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

#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <string>

/*ERROR_SCOPE*/
#define TRANSFER "TRANSFER"
#define DESTINATION "DESTINATION"
#define AGENT "GENERAL_FAILURE"
#define SOURCE "SOURCE"

/*ERROR_PHASE*/
#define TRANSFER_PREPARATION "TRANSFER_PREPARATION"
#define TRANSFER "TRANSFER"
#define TRANSFER_FINALIZATION "TRANSFER_FINALIZATION"
#define ALLOCATION "ALLOCATION"
#define TRANSFER_SERVICE "TRANSFER_SERVICE"

/*REASON_CLASS*/
#define USER_ERROR "USER_ERROR"
#define INTERNAL_ERROR "INTERNAL_ERROR"
#define CONNECTION_ERROR "CONNECTION_ERROR"
#define REQUEST_TIMEOUT "REQUEST_TIMEOUT"
#define LOCALITY "LOCALITY"
#define ABORTED "ABORTED"
#define GRIDFTP_ERROR "GRIDFTP_ERROR"
#define HTTP_TIMEOUT "HTTP_TIMEOUT"
#define INVALID_PATH "INVALID_PATH"
#define STORAGE_INTERNAL_ERROR ""
#define GENERAL_FAILURE "GENERAL_FAILURE"
#define SECURITY_ERROR "SECURITY_ERROR"

#include <cstring>

/**
 * Return the string representation of the error code,
 * replacing spaces with '_'
 */
inline std::string mapErrnoToString(int err)
{
    if (err != 0)
        {
            char buf[256] = {0};
            char const *str = strerror_r(err, buf, sizeof(buf));
            if (str)
                {
                    std::string rep(str);
                    std::replace(rep.begin(), rep.end(), ' ', '_');
                    return boost::to_upper_copy(rep);
                }
        }
    return "GENERAL ERROR";
}
