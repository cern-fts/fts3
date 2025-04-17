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
#include <chrono>
#include <decaf/lang/System.h>
#include <activemq/core/ActiveMQConnection.h>

#include "BrokerPublisher.h"
#include "MonitoringMessage.h"
#include "BrokerConnection.h"
#include "common/Logger.h"
#include "common/ConcurrentQueue.h"

using namespace fts3::common;

BrokerPublisher::BrokerPublisher(const BrokerConfig& config, MessageRemover& msgRemover, std::stop_token token) :
                                 stop_token(token), brokerConfig(config), msgRemover(msgRemover), connected(false)
{
    // Set properties for SSL, if enabled
    if (brokerConfig.UseSSL()) {
        const std::string clientKeystore = brokerConfig.GetClientKeyStore();
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Using clientKeystore " << clientKeystore << commit;
        decaf::lang::System::setProperty("decaf.net.ssl.keyStore", clientKeystore);
        if (!brokerConfig.GetClientKeyStorePassword().empty()) {
            const std::string keyStorePassword = brokerConfig.GetClientKeyStorePassword();
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Using keyStorePassword " << keyStorePassword << commit;
            decaf::lang::System::setProperty("decaf.net.ssl.keyStorePassword", keyStorePassword);
        }

        const std::string trustStore = brokerConfig.GetRootCA();
        decaf::lang::System::setProperty("decaf.net.ssl.trustStore", brokerConfig.GetRootCA());
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Using trustStore " << trustStore << commit;
        if (brokerConfig.SslVerify()) {
            decaf::lang::System::setProperty("decaf.net.ssl.disablePeerVerification", "false");
        } else {
            FTS3_COMMON_LOGGER_LOG(INFO, "Disable peer verification");
            decaf::lang::System::setProperty("decaf.net.ssl.disablePeerVerification", "true");
        }
    } else {
        FTS3_COMMON_LOGGER_LOG(INFO, "Not using SSL");
    }
}

BrokerPublisher::~BrokerPublisher()
{
}

bool BrokerPublisher::sendMessage(MonitoringMessageCallback &msg)
{
    bool message_delivered = false;
    try {
        message_delivered = current_producer->sendMessage(msg);
    } catch (const cms::UnsupportedOperationException &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerPublsher: cms::UnsupportedOperationException: " << ex.what() << commit;
    } catch (const cms::InvalidDestinationException &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerPublisher: cms::InvalidDestinationException: " << ex.what() << commit;
    } catch (const cms::CMSException &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerPublisher: cms::CMSException: " << ex.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_LOG(DEBUG, "Unknown exception occurred while sending a message to a!");
    };

    if (message_delivered == false) {
        msg.state = MonitoringMessageCallback::failed;
        current_producer = MsgProducers.erase(current_producer);
    }

    if (current_producer == MsgProducers.end() || std::next(current_producer) == MsgProducers.end()) {
        current_producer = MsgProducers.begin();
    } else {
        ++current_producer;
    }

    return message_delivered;
}

bool BrokerPublisher::refresh_sessions()
{
    FTS3_COMMON_LOGGER_LOG(DEBUG, brokerConfig.GetBrokerURI());
    try {
        const std::string broker_alias = brokerConfig.GetBroker();
        std::vector<std::string> hostnames = brokerConfig.resolve_dns_alias(broker_alias);
        if (hostnames.empty()) {
            FTS3_COMMON_LOGGER_LOG(ERR, "Unable to resolve '" + broker_alias + "', no ActiveMQ broker(s) found.");
            return false;
        }

        {
            std::string resolved_hostnames;
            for (std::vector<std::string>::const_iterator it = hostnames.begin(); it != hostnames.end(); ++it) {
                resolved_hostnames.append(*it);
                if (std::next(it) != hostnames.end()) {
                    resolved_hostnames.append(", ");
                }
            }
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Resolved hostnames: " << resolved_hostnames << commit;
        }

        // Remove active connections that are not anymore in the DNS alias
        for (auto it = MsgProducers.begin(); it != MsgProducers.end(); ++it) {
            auto msg_it = std::find(hostnames.begin(), hostnames.end(), (*it).getBrokerName());
            if (msg_it == hostnames.end()) {
                it = MsgProducers.erase(it);
            } else {
                hostnames.erase(msg_it);
            }
        }

        if (!MsgProducers.empty()) {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Broker connection keep active: " << MsgProducers << commit;
        }

        FTS3_COMMON_LOGGER_LOG(DEBUG, "Connection string " + brokerConfig.GetConnectionString(broker_alias));
        std::string brokers_unreachable;
        std::string brokers_connected;
        bool fresh_connections = false;
        for (auto broker = hostnames.begin(); broker != hostnames.end(); ++broker) {
            if (stop_token.stop_requested()) {
                FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "MessageRemoverThread: stop requested!" << commit;
                break;
            }

            const std::string connectionString = brokerConfig.GetConnectionString(*broker);

            // Instantiate and connects a message producer to an ActiveMQ broker.
            try {
                MsgProducers.emplace_back(*broker, brokerConfig, connectionString);
                if (!brokers_connected.empty()) {
                    brokers_connected.append(", ");
                }
                brokers_connected.append(*broker);
                fresh_connections = true;
                continue;
            } catch (const cms::InvalidDestinationException &ex) {
                FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerConnection: Unable to connect to '" << *broker
                                                 << "' (InvalidDestinationException: " << ex.what() << ")." << commit;
            } catch (const cms::CMSException &ex) {
                FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerConnection: Unable to connect to '" << *broker
                                                 << "' (CMSException: " << ex.what() << ")." << commit;
            }

            if (!brokers_unreachable.empty())
                brokers_unreachable.append(", ");
            brokers_unreachable.append(*broker);
        }

        if (!brokers_unreachable.empty()) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Unable to connect to the broker(s) '" << brokers_unreachable << "'." << commit;

            if (MsgProducers.empty()) {
                return false;
            }
        }
        current_producer = MsgProducers.begin();

        if (fresh_connections) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "New broker connections has been established with: " << brokers_connected
                                             << " resolved via " << broker_alias << " alias." << commit;
        }

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerPublisher: Active brokers connections: " << MsgProducers << commit;
        return true;
    } catch (const cms::CMSException &ex) {
        FTS3_COMMON_LOGGER_LOG(ERR, ex.getMessage());
    } catch (...) {
        FTS3_COMMON_LOGGER_LOG(ERR, "BrokerPublisher: Unknown exception happened in '" << __func__ << "'.");
    }

    return false;
}

void BrokerPublisher::collect_messages()
{
    auto &monitoring_messages = ConcurrentQueue<std::unique_ptr<std::vector<MonitoringMessage>>>::getInstance();
    for (unsigned int i = 0; i < 10; ++i) {
        const bool wait_for_messages = messages.empty();
        std::unique_ptr<std::vector<MonitoringMessage>> vector = monitoring_messages.pop(wait_for_messages);
        if (!vector) {
            break;
        }

        for (auto &message : *vector) {
            if (message.message.empty() || message.dirq_iter.empty()) {
                continue;
            }

            try {
                messages.emplace_back(std::make_unique<MonitoringMessageCallback>(std::move(message)));
            } catch (std::bad_alloc &ex) {
                FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Unable to allocate MonitoringMessageCallback: " << ex.what() << commit;
            };
        }
    }
}

void BrokerPublisher::dispatch_messages()
{
    for (auto msg = messages.begin(); msg != messages.end(); ) {
        std::unique_ptr<MonitoringMessageCallback>& monitoring_msg = *msg;
        switch (monitoring_msg->state) {
            case MonitoringMessageCallback::MessageState::ready:
                if (!stop_token.stop_requested()) {
                    monitoring_msg->state = MonitoringMessageCallback::MessageState::sending;
                    if (!sendMessage(*monitoring_msg) && MsgProducers.empty()) {
                        msg = messages.end();
                        connected = false;
                    } else {
                        std::advance(msg, 1);
                    }
                } else {
                    msg = messages.erase(msg);
                }
                break;
            case MonitoringMessageCallback::MessageState::sending:
                std::advance(msg, 1);
                break;
            case MonitoringMessageCallback::MessageState::delivered:
                if (messages_to_remove->size() < 100) {
                    messages_to_remove->emplace_back(std::move(monitoring_msg));
                } else {
                    msgRemover.push(std::move(messages_to_remove));
                    messages_to_remove = std::make_unique<std::list<std::unique_ptr<MonitoringMessageCallback>>>();
                    messages_to_remove->emplace_back(std::move(monitoring_msg));
                }
                msg = messages.erase(msg);
                break;
            case MonitoringMessageCallback::MessageState::failed:
                FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Unable to deliver" << commit;
                if (!stop_token.stop_requested()) {
                    monitoring_msg->state = MonitoringMessageCallback::MessageState::ready;
                    if (MsgProducers.empty()) {
                        msg = messages.end();
                        connected = false;
                    }
                } else {
                    msg = messages.erase(msg);
                }
        }
    }

    if (messages_to_remove->size()) {
        msgRemover.push(std::move(messages_to_remove));
        messages_to_remove = std::make_unique<std::list<std::unique_ptr<MonitoringMessageCallback>>>();
    }
}

void BrokerPublisher::start()
{
    pthread_setname_np(pthread_self(), "fts-msg-loader");

    auto start_time = std::chrono::steady_clock::now();
    try {
        messages_to_remove = std::make_unique<std::list<std::unique_ptr<MonitoringMessageCallback>>>();
    } catch (std::bad_alloc &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Unable to allocate list, not enough memory?" << commit;
    }

    for (;;) {
        try {
            // Establish ActiveMQ broker connection based on DNS alias resolution, scan x minutes for changes on the DNS alias.
            const auto session_timestamp = std::chrono::steady_clock::now();
            const auto time_elapsed = std::chrono::duration_cast<std::chrono::minutes>(session_timestamp - start_time);
            const auto broker_check_interval = brokerConfig.GetBrokerCheckInterval();
            while ((!connected || time_elapsed.count() >= broker_check_interval) && !stop_token.stop_requested()) {
                connected = refresh_sessions();
                if (!connected) {
                    std::condition_variable_any cv;
                    std::mutex mtx;
                    const unsigned int retry_delay = 10;
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Unable to connect to the ActiveMQ broker(s) provided by '"
                                                     << brokerConfig.GetBroker() << "' alias." << commit;
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BrokerPublisher: Attempt to re-establish the connections to '"
                                                     << brokerConfig.GetBroker() << "' in " << retry_delay << " seconds." << commit;
                    cv.wait_for(mtx, stop_token, std::chrono::seconds(retry_delay), [] {return false;});
                    continue;
                }

                start_time = std::chrono::steady_clock::now();
                break;
            }

            // Get the messages loaded in memory from the MessageLoader thread. Prepare MonitoringMessageCallback's for async dispatch
            if (!stop_token.stop_requested()) {
                BrokerPublisher::collect_messages();
            }

            // Round-robin asyncronous dispatch to ActiveMQ brokers
            BrokerPublisher::dispatch_messages();
        } catch (const cms::CMSException &ex) {
            FTS3_COMMON_LOGGER_LOG(ERR, ex.getMessage());
            connected = false;
        } catch (const std::exception &ex) {
            FTS3_COMMON_LOGGER_LOG(ERR, ex.what());
            connected = false;
        } catch (...) {
            FTS3_COMMON_LOGGER_LOG(CRIT, "Unexpected exception");
            connected = false;
        }

        if (stop_token.stop_requested() && messages.empty()) {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Leaving BrokerPublisher Thread" << commit;
            break;
        }
    }
}
