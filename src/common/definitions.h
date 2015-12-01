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
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cmath>
#include <cstddef>
#include <boost/date_time/posix_time/posix_time_types.hpp>


#define JOB_ID_LEN 36+1
#define FILE_ID_LEN 36
#define TRANFER_STATUS_LEN 50
#define TRANSFER_MESSAGE 1024
#define MAX_NUM_MSGS 50000
#define SOURCE_SE_ 100
#define DEST_SE_ 100


/**
 * Return MegaBytes per second from the given transferred bytes and duration
 */
inline double convertBtoM(double byte,  double duration)
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
    return throughput != 0.0? pround((throughput / 1024), 3): 0.0;
}


inline boost::posix_time::time_duration::tick_type milliseconds_since_epoch()
{
    using boost::gregorian::date;
    using boost::posix_time::ptime;
    using boost::posix_time::microsec_clock;

    static ptime const epoch(date(1970, 1, 1));
    return (microsec_clock::universal_time() - epoch).total_milliseconds();
}


#define DEFAULT_TIMEOUT 4000
#define MID_TIMEOUT 4000
const int timeouts[] = {4000};
const size_t timeoutslen = (sizeof (timeouts) / sizeof *(timeouts));

#define DEFAULT_NOSTREAMS 4
const int nostreams[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
const size_t nostreamslen = (sizeof (nostreams) / sizeof *(nostreams));


#define DEFAULT_BUFFSIZE 0
const int buffsizes[] = {1048576, 4194304, 5242880, 7340032, 8388608, 9437184, 11534336, 12582912, 14680064, 67108864};
const size_t buffsizeslen = (sizeof (buffsizes) / sizeof *(buffsizes));

/*low active / high active / jobs / files*/
const int mode_1[] = {2,4,3,5};
const int mode_2[] = {4,6,5,8};
const int mode_3[] = {6,8,7,10};

#endif // DEFINITIONS_H
