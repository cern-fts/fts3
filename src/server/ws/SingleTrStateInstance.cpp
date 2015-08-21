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

#include "SingleTrStateInstance.h"
#include "logger.h"
#include "error.h"
#include "producer_consumer_common.h"
#include "db/generic/SingleDbInstance.h"
#include "config/serverconfig.h"
#include <sstream>

using namespace db;
using namespace FTS3_CONFIG_NAMESPACE;

std::string _getTrTimestampUTC()
{
    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);
    time_t msec = mktime(&tTime) * 1000; //the number of milliseconds since the epoch
    std::ostringstream oss;
    oss << fixed << msec;
    return oss.str();
}



std::unique_ptr<SingleTrStateInstance> SingleTrStateInstance::i;
boost::mutex SingleTrStateInstance::_mutex;


// Implementation

SingleTrStateInstance::SingleTrStateInstance(): monitoringMessages(true)
{
    std::string monitoringMessagesStr = theServerConfig().get<std::string > ("MonitoringMessaging");
    if(monitoringMessagesStr == "false")
        monitoringMessages = false;

    ftsAlias = theServerConfig().get<std::string > ("Alias");
}

SingleTrStateInstance::~SingleTrStateInstance()
{
}


void SingleTrStateInstance::sendStateMessage(const std::string& jobId, int fileId)
{

    if(!monitoringMessages)
        return;

    std::vector<struct message_state> files;
    try
        {
            if(fileId != -1)  //both job_id and file_id are provided
                {
                    files  = db::DBSingleton::instance().getDBObjectInstance()->getStateOfTransfer(jobId, fileId);
                    if(!files.empty())
                        {
                            std::vector<struct message_state>::iterator it;
                            for (it = files.begin(); it != files.end(); ++it)
                                {
                                    struct message_state tmp = (*it);
                                    constructJSONMsg(&tmp);
                                }
                        }
                }
            else   //need to get file_id for the given job
                {
                    files = db::DBSingleton::instance().getDBObjectInstance()->getStateOfTransfer(jobId, -1);
                    if(!files.empty())
                        {
                            std::vector<struct message_state>::iterator it;
                            for (it = files.begin(); it != files.end(); ++it)
                                {
                                    struct message_state tmp = (*it);
                                    constructJSONMsg(&tmp);
                                }
                        }
                }
        }
    catch (Err& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Failed saving transfer state, " << e.what() << commit;
        }
    catch (std::exception& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Failed saving transfer state, " << ex.what() << commit;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Failed saving transfer state " << commit;
        }
}

void SingleTrStateInstance::sendStateMessage(  const std::string&  vo_name, const std::string&  source_se, const std::string&  dest_se, const std::string&  job_id, int file_id, const std::string&
        job_state, const std::string&  file_state, int retry_counter, int retry_max, const std::string&  job_metadata, const std::string&  file_metadata)
{

    if(!monitoringMessages)
        return;

    struct message_state state;
    state.vo_name = vo_name;
    state.source_se = source_se;
    state.dest_se = dest_se;
    state.job_id = job_id;
    state.file_id = file_id;
    state.job_state = job_state;
    state.file_state = file_state;
    state.retry_counter = retry_counter;
    state.retry_max = retry_max;
    state.job_metadata = job_metadata;
    state.file_metadata = file_metadata;

    constructJSONMsg(&state);
}


void SingleTrStateInstance::constructJSONMsg(const struct message_state* state)
{

    if(!monitoringMessages)
        return;

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
            strncpy(message.msg, std::string(json_message.str()).c_str(), sizeof(message.msg));
            message.msg[sizeof(message.msg) - 1] = '\0';
            runProducerMonitoring( message );
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Message cannot be sent, check length: " << json_message.str().length() << commit;
        }
}




