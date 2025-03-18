/*
 * Copyright (c) CERN 2013-2025
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
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/Message.h>
#include <cms/MapMessage.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/util/Config.h>
#include <activemq/library/ActiveMQCPP.h>
#include <activemq/commands/Command.h>
#include <activemq/transport/TransportListener.h>

#include "msg-bus/consumer.h"
#include "MonitoringMessage.h"
#include "BrokerConfig.h"

class MonitoringMessageCallback : public cms::AsyncCallback {
    public:
        MonitoringMessageCallback(MonitoringMessage&& message);
        ~MonitoringMessageCallback();

        enum MessageState {ready, sending, delivered, failed};
        std::atomic<MessageState> state;

        void onSuccess();
        void onException(const cms::CMSException& ex);

        MonitoringMessage message;
};

class BrokerConnection {
    private:
        const std::string brokerName;
        const BrokerConfig& brokerConfig;
        const std::string FTSEndpoint;
        const std::string FQDN;

        std::unique_ptr<activemq::core::ActiveMQConnectionFactory> connectionFactory = nullptr;
        std::unique_ptr<cms::Connection> connection = nullptr;
        std::unique_ptr<cms::Session> session = nullptr;

        std::unique_ptr<const cms::Destination> destination_transfer_started = nullptr;
        std::unique_ptr<const cms::Destination> destination_transfer_completed = nullptr;
        std::unique_ptr<const cms::Destination> destination_transfer_state = nullptr;
        std::unique_ptr<const cms::Destination> destination_optimizer = nullptr;

        std::unique_ptr<cms::MessageProducer> producer_transfer_started = nullptr;
        std::unique_ptr<cms::MessageProducer> producer_transfer_completed = nullptr;
        std::unique_ptr<cms::MessageProducer> producer_transfer_state = nullptr;
        std::unique_ptr<cms::MessageProducer> producer_optimizer = nullptr;

    public:
        BrokerConnection(const std::string &brokerName, const BrokerConfig &config, const std::string &connectionString);
        BrokerConnection(BrokerConnection &&other);
        ~BrokerConnection();

        bool sendMessage(MonitoringMessageCallback &cb) const;
        const std::string& getBrokerName() const;
};

std::ostream& operator << (std::ostream& os, const std::vector<BrokerConnection>& vec);
std::ostream& operator << (std::ostream& os, const std::list<BrokerConnection>& list);
