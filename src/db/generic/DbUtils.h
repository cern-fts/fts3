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

#include <netdb.h>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

namespace db
{

/**
 * Returns the current time, plus the difference specified
 * in advance, in UTC time
 */
inline time_t getUTC(int advance)
{
    time_t now;
    if(advance > 0)
        now = time(NULL) + advance;
    else
        now = time(NULL);

    struct tm *utc;
    utc = gmtime(&now);
    return timegm(utc);
}

/**
 * Get timestamp in milliseconds, UTC, as a string
 */
static inline std::string getStrUTCTimestamp()
{
    //the number of milliseconds since the epoch
    time_t msec = getUTC(0) * 1000;
    std::ostringstream oss;
    oss << std::fixed << msec;
    return oss.str();
}

/**
 * Return MegaBytes per second from the given transferred bytes and duration
 */
inline double convertBtoM( double byte,  double duration)
{
    return ((((byte / duration) / 1024) / 1024) * 100) / 100;
}


/**
 * Round a double with a given number of digits
 */
inline double pround(double x, unsigned int digits)
{
    double shift = pow(10, digits);
    return rint(x * shift) / shift;
}

/**
 * Return MegaBytes per second given throughput in KiloBytes per second
 */
inline double convertKbToMb(double throughput)
{
    return throughput != 0.0? pround((throughput / 1024), 6): 0.0;
}

/**
 * From a transfer parameters string, return the timeout
 */
inline int extractTimeout(std::string & str)
{
    size_t found;
    found = str.find("timeout:");
    if (found != std::string::npos)
        {
            str = str.substr(found, str.length());
            size_t found2;
            found2 = str.find(",buffersize:");
            if (found2 != std::string::npos)
                {
                    str = str.substr(0, found2);
                    str = str.substr(8, str.length());
                    return atoi(str.c_str());
                }

        }
    return 0;
}

/**
 * Return the full qualified hostname
 */
inline std::string getFullHostname()
{
    char hostname[MAXHOSTNAMELEN] = {0};
    gethostname(hostname, sizeof(hostname));

    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    // First is OK
    if (getaddrinfo(hostname, NULL, &hints, &info) == 0)
        {
            strncpy(hostname, info->ai_canonname, sizeof(hostname));
            freeaddrinfo(info);
        }
    return hostname;
}

inline int extractStreams(std::string & str)
{
    size_t found;
    found = str.find("nostreams:");
    if (found != std::string::npos)
        {
            size_t found2;
            found2 = str.find(",timeout:");
            if (found2 != std::string::npos)
                {
                    str = str.substr(0, found2);
                    str = str.substr(10, str.length());
                    return atoi(str.c_str());
                }
        }
    return 0;
}


}

