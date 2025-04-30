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

#include <memory>
#include <json/json.h>
#include <decaf/lang/System.h>

#include "BrokerConnection.h"
#include "common/Logger.h"
#include "common/Uri.h"

using namespace fts3::common;

// Originally, this was a End-Of-Transmission character (0x04)
// which makes the messages invalid json.
// Starting with FTS 3.6, we replace this with a space, so code that explicitly
// strips the last byte regardless of its value (i.e. Rucio) still works, but
// making the message now a valid json.
// See https://its.cern.ch/jira/browse/FTS-827
// Of course, it would be nicer to get rid of this trailing byte altogether, but need to keep
// compatibility until people stops dropping the last byte.
static const char EOT = ' ';


BrokerConnection::BrokerConnection(const std::string &brokerName, const BrokerConfig &config, const std::string &connectionString)
        : brokerName(brokerName), brokerConfig(config), FTSEndpoint(brokerConfig.GetFTSEndpoint()),
          FQDN(getFullHostname()), connectionFactory(std::make_unique<activemq::core::ActiveMQConnectionFactory>(connectionString,
          brokerConfig.GetUserName(), brokerConfig.GetPassword())), connection(connectionFactory->createConnection()),
          session(connection->createSession(cms::Session::AUTO_ACKNOWLEDGE))
{
    connection->start();

    // Prepare the different destination
    if (brokerConfig.UseTopics()) {
        destination_transfer_started.reset(session->createTopic(brokerConfig.GetStartDestination()));

        destination_transfer_completed.reset(session->createTopic(brokerConfig.GetCompleteDestination()));

        destination_transfer_state.reset(session->createTopic(brokerConfig.GetStateDestination()));

        destination_optimizer.reset(session->createTopic(brokerConfig.GetOptimizerDestination()));
        FTS3_COMMON_LOGGER_LOG(DEBUG, "ActiveMQ use topics");
    } else {
        destination_transfer_started.reset(session->createQueue(brokerConfig.GetStartDestination()));

        destination_transfer_completed.reset(session->createQueue(brokerConfig.GetCompleteDestination()));

        destination_transfer_state.reset(session->createQueue(brokerConfig.GetStateDestination()));

        destination_optimizer.reset(session->createQueue(brokerConfig.GetOptimizerDestination()));
        FTS3_COMMON_LOGGER_LOG(DEBUG, "ActiveMQ use queues");
    }

    const int ttlMs = brokerConfig.GetTTL() * 3600000;

    producer_transfer_started.reset(session->createProducer(destination_transfer_started.get()));
    producer_transfer_started->setDeliveryMode(cms::DeliveryMode::PERSISTENT);
    producer_transfer_started->setTimeToLive(ttlMs);

    producer_transfer_completed.reset(session->createProducer(destination_transfer_completed.get()));
    producer_transfer_completed->setDeliveryMode(cms::DeliveryMode::PERSISTENT);
    producer_transfer_completed->setTimeToLive(ttlMs);

    producer_transfer_state.reset(session->createProducer(destination_transfer_state.get()));
    producer_transfer_state->setDeliveryMode(cms::DeliveryMode::PERSISTENT);
    producer_transfer_state->setTimeToLive(ttlMs);

    producer_optimizer.reset(session->createProducer(destination_optimizer.get()));
    producer_optimizer->setDeliveryMode(cms::DeliveryMode::NON_PERSISTENT);
    producer_optimizer->setTimeToLive(ttlMs);
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerConnection(" << getBrokerName() << "): connected!" << commit;
}

BrokerConnection::~BrokerConnection()
{
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerConnection(" << getBrokerName() << "): closing connection!" << commit;
    if (connection.get()) {
        try {
            connection->close();
        } catch (const cms::CMSException &ex) {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerConnection(" << getBrokerName()
                                             << "): CMSException occurred " << ex.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerConnection(" << getBrokerName()
                                             << "): Unknown exception occurred while closing the connection with "
                                             " the ActiveMQ broker." << commit;
        };
    }
}

MonitoringMessageCallback::MonitoringMessageCallback(MonitoringMessage&& msg)
    : state(ready), message(std::move(msg))
{
}

MonitoringMessageCallback::~MonitoringMessageCallback()
{
}

void MonitoringMessageCallback::onSuccess()
{
    state = delivered;
}

void MonitoringMessageCallback::onException(const cms::CMSException& ex)
{
    state = failed;
    FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Message delivery failed: " << ex.what() << commit;
}

bool BrokerConnection::sendMessage(MonitoringMessageCallback &cb) const
{
    const std::string& rawMsg = cb.message.message;
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "rawMsg:" << rawMsg << commit;
    const std::string type = rawMsg.substr(0, 2);

    // Modify on the fly to add the endpoint
    Json::Value msg;

    std::istringstream input(rawMsg.substr(3));
    input >> msg;

    msg["endpnt"] = FTSEndpoint;

    if (brokerConfig.PublishFQDN()) {
        msg["fqdn"] = FQDN;
    }

    std::ostringstream output;
    output << msg;

    // Add EOT character
    output << EOT;

    // Create message and set VO attribute if available
    std::unique_ptr<cms::TextMessage> message = nullptr;
    try {
        message.reset(session->createTextMessage(output.str()));
    } catch (const cms::CMSException &e) {
        return false;
    };

    if (msg.isMember("vo_name")) {
        message->setStringProperty("vo", msg.get("vo_name", "").asString());
    }

    // Route
    if (type == "ST") {
        producer_transfer_started->send(message.get(), &cb);
        auto transferId = msg.get("transfer_id", "").asString();

        if (!transferId.empty()) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Start message: " << transferId << commit;
        }
    } else if (type == "CO") {
        producer_transfer_completed->send(message.get(), &cb);
        auto transferId = msg.get("tr_id", "").asString();

        if (!transferId.empty()) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Completion message: " << transferId << commit;
        }
    } else if (type == "SS") {
        producer_transfer_state->send(message.get(), &cb);
        auto state = msg.get("file_state", "INVALID").asString();
        auto jobId = msg.get("job_id", "").asString();

        if (!jobId.empty() && msg.isMember("file_id")) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "State change: " << state << " " << jobId << "/"
                                            << msg.get("file_id", 0).asUInt64() << commit;
        }
    } else if (type == "OP") {
        producer_optimizer->send(message.get(), &cb);
        auto sourceSe = msg.get("source_se", "").asString();
        auto destSe = msg.get("dest_se", "").asString();

        if (!sourceSe.empty() && !destSe.empty()) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer update: " << sourceSe << " => " << destSe << commit;
        }
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Dropping unknown message type: " << type << commit;
    }

    return true;
}


BrokerConnection::BrokerConnection(BrokerConnection&& other) :
                                    brokerName(std::move(other.brokerName)),
                                    brokerConfig(std::move(other.brokerConfig)),
                                    FTSEndpoint(std::move(other.FTSEndpoint)),
                                    FQDN(std::move(other.FQDN)),
                                    connectionFactory(std::move(other.connectionFactory)),
                                    connection(std::move(other.connection)),
                                    session(std::move(other.session)),
                                    destination_transfer_started(std::move(other.destination_transfer_started)),
                                    destination_transfer_completed(std::move(other.destination_transfer_completed)),
                                    destination_transfer_state(std::move(other.destination_transfer_state)),
                                    destination_optimizer(std::move(other.destination_optimizer)),
                                    producer_transfer_started(std::move(other.producer_transfer_started)),
                                    producer_transfer_completed(std::move(other.producer_transfer_completed)),
                                    producer_transfer_state(std::move(other.producer_transfer_state)),
                                    producer_optimizer(std::move(other.producer_optimizer))
{
}

const std::string& BrokerConnection::getBrokerName() const
{
    return brokerName;
}

std::ostream& operator << (std::ostream& os, const std::vector<BrokerConnection>& vec)
{
    for (std::vector<BrokerConnection>::const_iterator i = vec.begin(); i != vec.end(); ++i) {
        os << (*i).getBrokerName();
        if (std::next(i) != vec.end()) {
            os << ", ";
        }
    }
    return os;
}

std::ostream& operator << (std::ostream& os, const std::list<BrokerConnection>& list)
{
    for (std::list<BrokerConnection>::const_iterator i = list.begin(); i != list.end(); ++i) {
        os << (*i).getBrokerName();
        if (std::next(i) != list.end()) {
            os << ", ";
        }
    }
    return os;
}
