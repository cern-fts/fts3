/**
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://www.eu-egee.org/partners/ for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * This following code is to used for generating information retrieved from the database
 * regarding the load of the channels, ratio of active/max, etc.
 * It is executed hourly
 */

#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/util/concurrent/CountDownLatch.h>
#include <decaf/lang/Long.h>
#include <decaf/util/Date.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/util/Config.h>
#include <activemq/library/ActiveMQCPP.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/MapMessage.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <vector>
#include <db/generic/SingleDbInstance.h>
#include "utility_routines.h"
#include "Logger.h"
#include <memory>
#include <exception>
#include "config/serverconfig.h"
#include "common/logger.h"
#include "common/error.h"

using namespace activemq;
using namespace activemq::core;
using namespace decaf;
using namespace decaf::lang;
using namespace decaf::util;
using namespace decaf::util::concurrent;
using namespace cms;
using namespace std;
using namespace FTS3_COMMON_NAMESPACE;
using namespace FTS3_CONFIG_NAMESPACE;


class MsgProducer :  public Runnable {
private:
    vector <std::string> credentials;
    string sql;
    string sql2;
    MonitoringDbIfce* monitoringDb;

    Connection* connection;
    Session* session;
    Destination* destination;
    MessageProducer* producer;

    std::string brokerURI;

    std::string broker;
    std::string queueName;
    std::string errorMessage;

    double active;
    double max;
    double ratio;

    bool connectionIsOK;


public:

    MsgProducer() {

        activemq::library::ActiveMQCPP::initializeLibrary();

        this->connection = NULL;
        this->session = NULL;
        this->destination = NULL;
        this->producer = NULL;

        this->active = 0.0;
        this->max = 0.0;
        this->ratio = 0.0;

        this->connectionIsOK = false;
    }

    virtual ~MsgProducer() {
        cleanup();
        activemq::library::ActiveMQCPP::shutdownLibrary();
    }

    virtual void run() {
        static int links_found = 0;
        char hostname[1024] = {0};
        gethostname(hostname, 1024);
        string host = hostname;

        try {
            bool fileExists = get_mon_cfg_file();

            if ((fileExists == false) || (false == getACTIVE()) || (false == getResolveAlias())) {
                logger::writeLog("Check the msg config file to see if the connection is active and the FQDN/alias of this machine", true);
                exit(0);
            }

            std::string dbUserName = theServerConfig().get<std::string>("DbUserName");
            std::string dbPassword = theServerConfig().get<std::string>("DbPassword");
            std::string dbConnectString = theServerConfig().get<std::string>("DbConnectString");

            try {
                this->monitoringDb = db::DBSingleton::instance().getMonitoringDBInstance();
                this->monitoringDb->init(dbUserName, dbPassword, dbConnectString);
            } catch (Err& exc) {
                logger::writeLog(std::string("Cannot connect to the database server: ") + exc.what());
                exit(1);
            }

            try {
                this->broker = getBROKER();
                this->queueName = getCRON();
                this->brokerURI = "tcp://" + broker + "?wireFormat=stomp&timeout=5000&maxReconnectAttempts=5&startupMaxReconnectAttempts=5";
            } catch (...) {
                logger::writeLog("Cannot read msg broker config file", true);
                exit(0);
            }


            // Create a ConnectionFactory
            std::auto_ptr<ConnectionFactory> connectionFactory(
                    ConnectionFactory::createCMSConnectionFactory(brokerURI));

            // Create a Connection
            if (true == getUSE_BROKER_CREDENTIALS())
                connection = connectionFactory->createConnection(getUSERNAME(), getPASSWORD());
            else
                connection = connectionFactory->createConnection();

            connection->start();

            session = connection->createSession(Session::AUTO_ACKNOWLEDGE);


            // Create the destination (Topic or Queue)
            if (getTOPIC()) {
                destination = session->createTopic(queueName);
            } else {
                destination = session->createQueue(queueName);
            }

            // Create a MessageProducer from the Session to the Topic or Queue
            producer = session->createProducer(destination);
            int ttl = GetIntVal(getTTL());
	    producer->setDeliveryMode( DeliveryMode::PERSISTENT );
            producer->setTimeToLive(ttl);

            // Create a message
            string text = "{";
            // fts instance identifier 
            text += "\"fts_id\":\"";
            text += "" + host + "";
            //text += ""; //null values
            text += "\"";
            // milliseconds since epoc time stamp of the report 
            text += ",\"time\":\"";
            text += _getTimestamp();
            text += "\",";

            std::vector<std::string> distinctVOs;
            monitoringDb->getVONames(distinctVOs);
            for (size_t voIndex = 0; voIndex < distinctVOs.size(); ++voIndex) {
                std::string& vo = distinctVOs[voIndex];

                //vo stuff
                text += "\"vo\":{";
                // Create an array of the channels
                text += "\"voname\":\"" + vo + "\",\"links\":[";

                // for every pair source_ dest_ host collect information
                std::vector<SourceAndDestSE> pairs;
                monitoringDb->getSourceAndDestSEForVO(vo, pairs);
                for (size_t pairIndex = 0; pairIndex < pairs.size(); ++pairIndex) {
                    SourceAndDestSE& pair = pairs[pairIndex];

                    ++links_found;
                    text += "{\"source_host\":\"";
                    text += pair.sourceStorageElement;
                    text += "\"";
                    text += ",\"dest_host\":\"";
                    text += pair.destinationStorageElement;
                    text += "\"";

                    unsigned active = monitoringDb->numberOfJobsInState(pair, "ACTIVE");
                    unsigned ready  = monitoringDb->numberOfJobsInState(pair, "READY");

                    text += ",\"active\":\"";
                    text += _to_string<unsigned>(active, std::dec);
                    text += "\"";
                    text += ",\"ratio\":\"";
                    text += _to_string<double>(ratio, std::dec);
                    text += "%\"";
                    text += ",\"ready\":\"";
                    text += _to_string<unsigned>(ready, std::dec);
                    text += "\"";
                    text += "},";
                }
                if (links_found > 0)
                    text.resize(text.size() - 1);

                text += "]},";

                links_found = 0;
                //text.resize(text.size() - 1);
            }

            text.resize(text.size() - 1);
            text += "}";
            text += 4; /*add EOT ctrl character*/


            TextMessage* message = session->createTextMessage(text);
            producer->send(message);

            logger::writeLog("PE " + text);
            delete message;
       }
       catch( const cms::CMSException& e) {
        std::string errorMessage = "Periodic msg producer cannot connect to broker " + broker + " : " + e.getMessage();
        logger::writeLog(errorMessage, true);
       }
       catch (const std::exception& e) {
            errorMessage = "Unrecovereable error occured: " + std::string(e.what());
            logger::writeLog(errorMessage, true);
       }
       catch (...) {
           logger::writeLog("Unexpected exception", true);

       }
    }

private:

    void cleanup() {

        // Destroy resources.
        try {
            if (destination != NULL) delete destination;
        } catch (CMSException& e) {
            e.printStackTrace();
        }
        destination = NULL;

        try {
            if (producer != NULL) delete producer;
        } catch (CMSException& e) {
            e.printStackTrace();
        }
        producer = NULL;

        // Close open resources.
        try {
            if (session != NULL) session->close();
            if (connection != NULL) connection->close();
        } catch (CMSException& e) {
            e.printStackTrace();
        }

        try {
            if (session != NULL) delete session;
        } catch (CMSException& e) {
            e.printStackTrace();
        }
        session = NULL;

        try {
            if (connection != NULL) delete connection;
        } catch (CMSException& e) {
            e.printStackTrace();
        }
        connection = NULL;

    }
};

int main(int argc, char** argv) {

    FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv);
    MsgProducer producer;

    // Start the producer thread.
    Thread producerThread(&producer);
    producerThread.start();

    // Wait for the threads to complete.
    producerThread.join();

    return 0;

}
