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

#include <json.h>
#include <memory>
#include "MsgProducer.h"
#include "utility_routines.h"
#include "concurrent_queue.h"
#include "Logger.h"
#include <signal.h>
#include "error.h"
#include "logger.h"
#include "serverconfig.h"
#include <sstream>

// End-Of-Transmission character
static const char EOT = 0x04;


bool stopThreads = false;

using namespace fts3::common; 


static std::string replaceMetadataString(std::string text)
{
    text = boost::replace_all_copy(text, "\\\"","\"");
    return text;
}



std::string started[] = {"agent_fqdn", "transfer_id", "endpnt", "timestamp", "src_srm_v", "dest_srm_v",
                    "vo", "src_url", "dst_url", "src_hostname", "dst_hostname", "src_site_name", "dst_site_name", "t_channel",
                    "srm_space_token_src", "srm_space_token_dst", "user_dn", "file_metadata", "job_metadata"
                   };

std::string startedToken[] = {"$a$", "$b$", "$c$", "$d$", "$e$", "$f$", "$g$", "$h$", "$i$", "$j$", "$k$", "$l$", "$m$", "$n$", "$o$", "$p$", "$q$", "$r$", "$s$"};

std::string completed[] = {"tr_id", "endpnt", "src_srm_v", "dest_srm_v", "vo", "src_url", "dst_url", "src_hostname", "dst_hostname", "src_site_name",
                      "dst_site_name", "t_channel", "timestamp_tr_st", "timestamp_tr_comp", "timestamp_chk_src_st", "timestamp_chk_src_ended", "timestamp_checksum_dest_st",
                      "timestamp_checksum_dest_ended", "t_timeout", "chk_timeout", "t_error_code", "tr_error_scope", "t_failure_phase", "tr_error_category", "t_final_transfer_state",
                      "tr_bt_transfered", "nstreams", "buf_size", "tcp_buf_size", "block_size", "f_size", "time_srm_prep_st", "time_srm_prep_end", "time_srm_fin_st", "time_srm_fin_end",
                      "srm_space_token_src", "srm_space_token_dst", "t__error_message", "tr_timestamp_start", "tr_timestamp_complete", "channel_type", "user_dn", "file_metadata", "job_metadata",
                      "retry","retry_max","job_m_replica", "job_state", "is_recoverable"
                     };

std::string completedToken[] =
{ "$a$", "$b$", "$c$", "$d$", "$e$", "$f$", "$g$", "$h$", "$i$", "$j$", "$k$",
        "$l$", "$m$", "$n$", "$o$", "$p$", "$q$", "$r$", "$s$", "$t$", "$u$",
        "$v$", "$w$", "$x$", "$y$", "$z$", "$0$", "$1$", "$2$", "$3$", "$4$",
        "$5$", "$6$", "$7$", "$8$", "$9$", "$10$", "$11$", "$12$", "$13$",
        "$14$", "$15$", "$16$", "$17$", "$18$", "$19$", "$20$", "$21$", "$22$" };




//utility routine private to this file
void find_and_replace(std::string &source, const std::string & find, std::string & replace)
{
    size_t j;
    for (; (j = source.find(find)) != std::string::npos;)
        {
            source.replace(j, find.length(), replace);
        }
}


MsgProducer::MsgProducer()
{
    connection = NULL;
    session = NULL;
    destination_transfer_started = NULL;
    destination_transfer_completed = NULL;
    producer_transfer_completed = NULL;
    producer_transfer_started = NULL;
    producer_transfer_state = NULL;
    destination_transfer_state = NULL;
    fts3::config::theServerConfig().read(0, NULL);
    FTSEndpoint = fts3::config::theServerConfig().get<std::string>("Alias");
    readConfig();
    connected = false;
}

MsgProducer::~MsgProducer()
{
}


static std::string _getVoFromMessage(const std::string& msg, const char* key)
{
    enum json_tokener_error error;
    json_object * jobj = json_tokener_parse_verbose(msg.c_str(), &error);
    if (!jobj)
        {
            throw Err_System(json_tokener_error_desc(error));
        }

    struct json_object *vo_name_obj = NULL;
    if (!json_object_object_get_ex(jobj, key, &vo_name_obj))
        {
            json_object_put(jobj);
            throw Err_System("Could not find vo_name in the message");
        }

    std::string vo_name = json_object_get_string(vo_name_obj);
    json_object_put(jobj);

    return vo_name;
}


void MsgProducer::sendMessage(std::string &temp)
{

    std::string tempFTS("");
    register int index = 0;

    if (temp.compare(0, 2, "ST") == 0)
            {
                for (index = 0; index < 19; index++)
                    find_and_replace(temp, startedToken[index], started[index]);
                temp = temp.substr(2, temp.length()); //remove message prefix
                tempFTS = "\"endpnt\":\"" + FTSEndpoint + "\"";
                find_and_replace(temp, "\"endpnt\":\"\"", tempFTS); //add FTS endpoint
                temp += EOT;
                TextMessage* message = session->createTextMessage(temp);
                producer_transfer_started->send(message);
                logger::writeLog(temp);
                delete message;
            }
        else if (temp.compare(0, 2, "CO") == 0)
            {
                for (index = 0; index < 49; index++)
                    {
                        find_and_replace(temp, completedToken[index], completed[index]);
                    }
                temp = temp.substr(2, temp.length()); //remove message prefix
                tempFTS = "\"endpnt\":\"" + FTSEndpoint + "\"";
                find_and_replace(temp, "\"endpnt\":\"\"", tempFTS); //add FTS endpoint

                temp = replaceMetadataString(temp);

                std::string vo;
                try
                    {
                        vo = _getVoFromMessage(temp, "vo");
                    }
                catch (const std::exception& e)
                    {
                        std::ostringstream error_message;
                        error_message << "(" << e.what() << ") " << temp;
                        LOGGER_ERROR(error_message.str());
                    }

                temp += EOT;
                TextMessage* message = session->createTextMessage(temp);
                message->setStringProperty("vo",vo);
                producer_transfer_completed->send(message);
                logger::writeLog(temp);
                delete message;
            }
        else if (temp.compare(0, 2, "SS") == 0)
            {
                temp = temp.substr(2, temp.length()); //remove message prefix

                temp = replaceMetadataString(temp);

                std::string vo;
                try
                    {
                        vo = _getVoFromMessage(temp, "vo_name");
                    }
                catch (const std::exception& e)
                    {
                        std::ostringstream error_message;
                        error_message << "(" << e.what() << ") " << temp;
                        LOGGER_ERROR(error_message.str());
                    }

                temp += EOT;
                TextMessage* message = session->createTextMessage(temp);
                message->setStringProperty("vo",vo);
                producer_transfer_state->send(message);
                logger::writeLog(temp);
                delete message;
            }
}

bool MsgProducer::getConnection()
{
    try
        {
            // Create a ConnectionFactory
            std::unique_ptr<ConnectionFactory> connectionFactory(
                ConnectionFactory::createCMSConnectionFactory(brokerURI));

            // Create a Connection
            if (true == getUSE_BROKER_CREDENTIALS())
                connection = connectionFactory->createConnection(getUSERNAME(), getPASSWORD());
            else
                connection = connectionFactory->createConnection();

            //connection->setExceptionListener(this);
            connection->start();

            session = connection->createSession(Session::AUTO_ACKNOWLEDGE);

            // Create the destination (Topic or Queue)
            if (getTOPIC())
                {
                    destination_transfer_started = session->createTopic(startqueueName);
                    destination_transfer_completed = session->createTopic(completequeueName);
                    destination_transfer_state = session->createTopic(statequeueName);
                }
            else
                {
                    destination_transfer_started = session->createQueue(startqueueName);
                    destination_transfer_completed = session->createQueue(completequeueName);
                    destination_transfer_state = session->createQueue(statequeueName);
                }

            int ttl = GetIntVal(getTTL());

            // Create a message producer
            producer_transfer_started = session->createProducer(destination_transfer_started);
            producer_transfer_started->setDeliveryMode(DeliveryMode::PERSISTENT);
            producer_transfer_started->setTimeToLive(ttl);

            producer_transfer_completed = session->createProducer(destination_transfer_completed);
            producer_transfer_completed->setDeliveryMode(DeliveryMode::PERSISTENT);
            producer_transfer_completed->setTimeToLive(ttl);

            producer_transfer_state = session->createProducer(destination_transfer_state);
            producer_transfer_state->setDeliveryMode(DeliveryMode::PERSISTENT);
            producer_transfer_state->setTimeToLive(ttl);

            connected = true;

        }
    catch (CMSException& e)
        {
            LOGGER_ERROR(e.getMessage());
            connected = false;
            sleep(10);
        }
    catch (...)
        {
            LOGGER_ERROR("Unknown exception");
            connected = false;
            sleep(10);
        }

    return true;
}

void MsgProducer::readConfig()
{
    bool fileExists = get_mon_cfg_file();

    if ((fileExists == false) || (false == getACTIVE()))
        {
            LOGGER_ERROR("Cannot read msg broker config file, or msg connection(ACTIVE=) is set to false");
            exit(0);
        }

    this->broker = getBROKER();
    this->startqueueName = getSTART();
    this->completequeueName = getCOMPLETE();
    this->statequeueName = getSTATE();

    this->logfilename = getLOGFILENAME();
    this->logfilepath = getLOGFILEDIR();

    this->brokerURI = "tcp://" + broker + "?wireFormat=stomp&soKeepAlive=true&wireFormat.MaxInactivityDuration=-1";
}

// If something bad happens you see it here as this class is also been
// registered as an ExceptionListener with the connection.
void MsgProducer::onException( const CMSException& ex AMQCPP_UNUSED)
{
    LOGGER_ERROR(ex.getMessage());
    stopThreads = true;
    std::queue<std::string> myQueue = concurrent_queue::getInstance()->the_queue;
    std::string ret;
    while(!myQueue.empty())
        {
            ret = myQueue.front();
            myQueue.pop();
            restoreMessageToDisk(ret);
        }
    connected = false;
    sleep(5);
}


void MsgProducer::run()
{
    std::string msg("");
    std::string msgBk("");
    while (stopThreads==false)
        {
            try
                {
                    if(!connected)
                        {
                            cleanup();
                            //https://issues.apache.org/jira/browse/AMQCPP-524
                            activemq::library::ActiveMQCPP::shutdownLibrary();
                            activemq::library::ActiveMQCPP::initializeLibrary();
                            getConnection();

                            if(!connected)
                                {
                                    sleep(10);
                                    continue;
                                }
                        }

                    //send messages
                    msg = concurrent_queue::getInstance()->pop();
                    msgBk = msg;
                    sendMessage(msg);
                    msg.clear();
                    usleep(100);
                }
            catch (CMSException& e)
                {
                    restoreMessageToDisk(msgBk);
                    LOGGER_ERROR(e.getMessage());
                    connected = false;
                    sleep(5);
                }
            catch (...)
                {
                    restoreMessageToDisk(msgBk);
                    LOGGER_ERROR("Unexpected exception");
                    connected = false;
                    sleep(5);
                }
        }
}

void MsgProducer::cleanup()
{

    // Destroy resources.
    try
        {
            if (destination_transfer_started != NULL) delete destination_transfer_started;
        }
    catch (CMSException& e)
        {
            e.printStackTrace();
        }
    destination_transfer_started = NULL;

    try
        {
            if (producer_transfer_started != NULL) delete producer_transfer_started;
        }
    catch (CMSException& e)
        {
            e.printStackTrace();
        }
    producer_transfer_started = NULL;

    try
        {
            if (destination_transfer_completed != NULL) delete destination_transfer_completed;
        }
    catch (CMSException& e)
        {
            e.printStackTrace();
        }
    destination_transfer_completed = NULL;

    try
        {
            if (producer_transfer_completed != NULL) delete producer_transfer_completed;
        }
    catch (CMSException& e)
        {
            e.printStackTrace();
        }
    producer_transfer_completed = NULL;


    try
        {
            if (destination_transfer_state != NULL) delete destination_transfer_state;
        }
    catch (CMSException& e)
        {
            e.printStackTrace();
        }
    destination_transfer_state = NULL;

    try
        {
            if (producer_transfer_state != NULL) delete producer_transfer_state;
        }
    catch (CMSException& e)
        {
            e.printStackTrace();
        }
    producer_transfer_state = NULL;




    // Close open resources.
    try
        {
            if (session != NULL) session->close();
        }
    catch (CMSException& e)
        {
            e.printStackTrace();
        }

    try
        {
            if (session != NULL) delete session;
        }
    catch (CMSException& e)
        {
            e.printStackTrace();
        }
    session = NULL;


    try
        {
            if (connection != NULL) connection->close();
        }
    catch (CMSException& e)
        {
            e.printStackTrace();
        }


    try
        {
            if (connection != NULL)
                delete connection;
        }
    catch (CMSException& e)
        {
            e.getStackTraceString();
        }
    connection = NULL;



}

