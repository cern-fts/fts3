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
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include "msg-bus/producer.h"

class MsgProducer : public decaf::lang::Runnable, public cms::ExceptionListener
{
private:

    cms::Connection* connection;
    cms::Session* session;
    cms::Destination* destination_transfer_started;
    cms::Destination* destination_transfer_completed;
    cms::MessageProducer* producer_transfer_started;
    cms::MessageProducer* producer_transfer_completed;
    cms::MessageProducer* producer_transfer_state;
    cms::Destination* destination_transfer_state;

    std::string brokerURI;
    std::string broker;
    std::string startqueueName;
    std::string completequeueName;
    std::string statequeueName;
    std::string FTSEndpoint;

    std::string logfilename;
    std::string logfilepath;
    bool getConnection();
    void readConfig();

public:
    MsgProducer(const std::string &localBaseDir);
    virtual ~MsgProducer();
    void sendMessage(std::string &text);
    virtual void run();
    void cleanup();
    virtual void onException(const cms::CMSException& ex AMQCPP_UNUSED);
    bool connected;

    Producer localProducer;
};
