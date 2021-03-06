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

#include <list>
#include <glib.h>
#include <netdb.h>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#include "config/ServerConfig.h"
#include "msg-bus/producer.h"
#include "GenericDbIfce.h"

using namespace fts3::config;

namespace db
{

const int DEFAULT_MIN_ACTIVE = 2;
const int DEFAULT_LAN_ACTIVE = 10;
const int DEFAULT_RETRY_DELAY = 120;
const int STREAMS_UPDATE_SAMPLE = 120;
const int STREAMS_UPDATE_MAX = 36000;
const int HIGH_THROUGHPUT = 50;
const int AVG_TRANSFER_DURATION = 15;
const int MAX_TRANSFER_DURATION = 3600;

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

}

