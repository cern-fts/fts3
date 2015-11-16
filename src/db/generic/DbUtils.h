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
#include "common/producer_consumer_common.h"
#include "GenericDbIfce.h"

using namespace fts3::config;

namespace db
{

int MAX_ACTIVE_PER_LINK = 60;
int MAX_ACTIVE_ENDPOINT_LINK = 60;
const int MIN_ACTIVE = 2;
const int LAN_ACTIVE = 10;
const int DEFAULT_RETRY_DELAY = 120;
const int STREAMS_UPDATE_SAMPLE = 120;
const int STREAMS_UPDATE_MAX = 36000;
const double EMA = 0.7;
const int MAX_SUCCESS_RATE = 100;
const int MED_SUCCESS_RATE = 98;
const int LOW_SUCCESS_RATE = 97;
const int BASE_SUCCESS_RATE = 96;
const int HIGH_THROUGHPUT = 50;
const int AVG_TRANSFER_DURATION = 15;
const int MAX_TRANSFER_DURATION = 3600;
// For bring online, this determines the maximum number of files per request
const int MAX_STAGING_BULK_SIZE = 1000;
// Do not send more than this number of concurrent requests
const int MAX_STAGING_CONCURRENT_REQUESTS = 200;


/*check if it's a multiple replicas job*/
inline bool is_mreplica(const std::list<SubmittedTransfer>& transfers)
{
    // if it has less than 2 pairs it wont be a m-replica
    if (transfers.size() < 2) return false;

    std::string destSurl = transfers.begin()->destination;

    for (auto iter = transfers.begin(); iter != transfers.end(); ++iter)
        {
            if(destSurl != iter->destination)
                {
                    return false;
                }
        }

    return true;
}

/*check if it's a m-hop job*/
inline bool is_mhop(const std::list<SubmittedTransfer>& transfers)
{
    // if it has less than 2 pairs it wont be a multi-hop
    if (transfers.size() < 2) return false;

    auto iter = transfers.begin();
    auto iter2 = transfers.begin();
    std::advance(iter2, 1);

    for (; iter2 != transfers.end(); ++iter, ++iter2)
        {
            if (iter->destination != iter2->source) return false;
        }

    return true;
}


/*borrowed from http://oroboro.com/irregular-ema/*/
inline double exponentialMovingAverage( double sample, double alpha, double cur )
{
    if(sample > 0)
        cur = ( sample * alpha ) + (( 1-alpha) * cur );
    return cur;
}


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


static inline void constructJSONMsg(struct message_state* state)
{
    bool monitoringMessages = true;
    std::string monitoringMessagesStr = ServerConfig::instance().get<std::string > ("MonitoringMessaging");
    if(monitoringMessagesStr == "false")
        monitoringMessages = false;

    if(!monitoringMessages)
        return;

    std::string ftsAlias = ServerConfig::instance().get<std::string > ("Alias");

    std::ostringstream json_message;
    json_message << "SS {";

    json_message << "\"endpnt\":" << "\"" << ftsAlias << "\",";
    json_message << "\"user_dn\":" << "\"" << state->user_dn << "\",";
    json_message << "\"src_url\":" << "\"" << state->source_url << "\",";
    json_message << "\"dst_url\":" << "\"" << state->dest_url << "\",";
    json_message << "\"vo_name\":" << "\"" << state->vo_name << "\",";
    json_message << "\"source_se\":" << "\"" << state->source_se << "\",";
    json_message << "\"dest_se\":" << "\"" << state->dest_se << "\",";
    json_message << "\"job_id\":" << "\"" << state->job_id << "\",";
    json_message << "\"file_id\":" << "\"" << state->file_id << "\",";
    json_message << "\"job_state\":" << "\"" << state->job_state << "\",";
    json_message << "\"file_state\":" << "\"" << state->file_state << "\",";
    json_message << "\"retry_counter\":" << "\"" << state->retry_counter << "\",";
    json_message << "\"retry_max\":" << "\"" << state->retry_max << "\",";
    if(state->job_metadata.length() > 0 )
        json_message << "\"job_metadata\":" << state->job_metadata << ",";
    else
        json_message << "\"job_metadata\":\"\",";
    if(state->file_metadata.length() > 0 )
        json_message << "\"file_metadata\":" << state->file_metadata << ",";
    else
        json_message << "\"file_metadata\":\"\",";
    json_message << "\"timestamp\":" << "\"" << state->timestamp << "\"";
    json_message << "}";

    struct message_monitoring message;

    if(json_message.str().length() < 3000)
        {
            g_strlcpy(message.msg, std::string(json_message.str()).c_str(), sizeof(message.msg));
            runProducerMonitoring( message );
        }
}


}

