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
#include <occi.h>
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
#include "utility_routines.h"
#include "db_routines.h"
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
    oracle::occi::Statement* stmt0;
    oracle::occi::ResultSet* rs0;
    oracle::occi::Statement* stmt;
    oracle::occi::ResultSet* rs;
    oracle::occi::Statement* stmt2;
    oracle::occi::ResultSet* rs2;
    oracle::occi::Statement* stmt3;
    oracle::occi::ResultSet* rs3;
    oracle::occi::Environment* env;
    oracle::occi::Connection* conn;

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

        // Create connection to DB
        this->stmt0 = NULL;
        this->rs0 = NULL;
        this->stmt = NULL;
        this->rs = NULL;
        this->stmt2 = NULL;
        this->rs2 = NULL;
        this->stmt3 = NULL;
        this->rs3 = NULL;
        this->env = NULL;
        this->conn = NULL;

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
                this->env = oracle::occi::Environment::createEnvironment();
                if (env)
                    this->conn = env->createConnection(dbUserName, dbPassword, dbConnectString);
                this->conn->setStmtCacheSize(100);
            } catch (const oracle::occi::SQLException& exc) {
                errorMessage = "Cannot connect to database server: " + exc.getMessage();
                logger::writeLog(errorMessage, true);
                exit(0);
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

            stmt0 = conn->createStatement("select distinct(vo_name) from T_JOB", "sta1");            
            stmt2 = conn->createStatement("select  distinct SOURCE_SE,DEST_SE from t_job where vo_name=:x");
	    stmt3 = conn->createStatement("Select Distinct Num1, Num2 From (Select Count(*) As Num1 From  T_Job Where Job_State='ACTIVE' And T_Job.Source_Se=:X  And	    T_Job.Dest_Se=:Y), (Select Count(*) As Num2  From T_Job where Job_state='READY' and t_job.SOURCE_SE=:z and t_job.DEST_SE=:e)", "sta3");

            if (conn) {
                rs0 = stmt0->executeQuery();
                while (rs0->next()) {
                    //vo stuff
                    text += "\"vo\":{";
                    // Create an array of the channels 
                    text += "\"voname\":\"" + rs0->getString(1) + "\",\"links\":[";
                    // Create a query to get channels names and types

                    stmt2->setString(1, rs0->getString(1));
                    rs2 = stmt2->executeQuery();                    
                        //  for every pair source_ dest_ host collecting information
                        while (rs2->next()) {
                            ++links_found;
                            text += "{\"source_host\":\"";
                            text += rs2->getString(1);
                            text += "\"";
                            text += ",\"dest_host\":\"";
                            text += rs2->getString(2);
                            text += "\"";

                            stmt3->setString(1, rs2->getString(1));
                            stmt3->setString(2, rs2->getString(2));
                            stmt3->setString(3, rs2->getString(1));
                            stmt3->setString(4, rs2->getString(2));
                            rs3 = stmt3->executeQuery();

                            while (rs3->next()) {
                                active = rs3->getInt(1);                              
                                text += ",\"active\":\"";
                                text += rs3->getString(1);
                                text += "\"";
                                text += ",\"ratio\":\"";
                                text += _to_string<double>(ratio, std::dec);
                                text += "%\"";
                                text += ",\"ready\":\"";
                                text += rs3->getString(2);
                                text += "\"";
                            }
                            stmt3->closeResultSet(rs3);
                            text += "},";
                        }
                        stmt2->closeResultSet(rs2);
                        if (links_found > 0)
                            text.resize(text.size() - 1);

                        text += "]},";

                        links_found = 0;
			text.resize(text.size() - 1);
                    
                    text += "},";

                }
                stmt0->closeResultSet(rs0);
                text.resize(text.size() - 1);

                text += "}";
                text += 4; /*add EOT ctrl character*/
			

                TextMessage* message = session->createTextMessage(text);
                producer->send(message);

                logger::writeLog("PE " + text);
                delete message;
            }
        }
       catch( const cms::CMSException& e) {
        std::string errorMessage = "Periodic msg producer cannot connect to broker " + broker + " : " + e.getMessage();
        logger::writeLog(errorMessage, true);
     	}
	catch (const oracle::occi::SQLException& exc) {
            errorMessage = exc.getMessage();
            logger::writeLog(errorMessage, true);
        }
       catch (const std::exception& e) {
            errorMessage = "Unrecovereable error occured: " + std::string(e.what());
            logger::writeLog(errorMessage, true);
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



        //now terminate oracle

        try {
            if (conn) {
                if (stmt0) {
                    //stmt0->closeResultSet(rs0);
                    conn->terminateStatement(stmt0);
                }                
                if (stmt2) {
                    //stmt2->closeResultSet(rs2);
                    conn->terminateStatement(stmt2);
                }
                if (stmt3) {
                    //stmt3->closeResultSet(rs3);
                    conn->terminateStatement(stmt3);
                }
                if (env) {
                    env->terminateConnection(conn);
                    oracle::occi::Environment::terminateEnvironment(env);
                }
            }

        } catch (const oracle::occi::SQLException& exc) {
            errorMessage = exc.getMessage();
            logger::writeLog(errorMessage, true);
        }

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
