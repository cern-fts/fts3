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
#include <algorithm>
#include <decaf/lang/System.h>
#include <activemq/core/ActiveMQConnection.h>

#include "BrokerPublisher.h"
#include "MonitoringMessage.h"
#include "BrokerConnection.h"
#include "common/Logger.h"
#include "common/ConcurrentQueue.h"

using namespace fts3::common;

BrokerPublisher::BrokerPublisher(const BrokerConfig& config, MessageRemover& msgRemover, std::stop_token token) :
                                 stop_token(token), brokerConfig(config), msgRemover(msgRemover)
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
    bool message_async_sent = false;
    if (current_producer != MsgProducers.end()) {
        try {
            message_async_sent = current_producer->sendMessage(msg);
        } catch (const cms::UnsupportedOperationException &ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BrokerPublisher: cms::UnsupportedOperationException: " << ex.what() << commit;
        } catch (const cms::InvalidDestinationException &ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BrokerPublisher: cms::InvalidDestinationException: " << ex.what() << commit;
        } catch (const cms::CMSException &ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BrokerPublisher: cms::CMSException: " << ex.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_LOG(ERR, "BrokerPublisher: Unknown exception occurred while sending a message to a!");
        };
    }

    if (message_async_sent == false) {
        msg.state = MonitoringMessageCallback::ready;
        if (current_producer != MsgProducers.end()) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Removing \"" << msg.broker_destination << "\" from the functional broker pool" << commit;
            current_producer = MsgProducers.erase(current_producer);
        }
    }

    if (MsgProducers.empty()) {
        return false;
    }

    if (current_producer == MsgProducers.end() || std::next(current_producer) == MsgProducers.end()) {
        current_producer = MsgProducers.begin();
    } else {
        ++current_producer;
    }

    return message_async_sent;
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

        // Remove dupicates, resolve_dns_alias can give the same node twice due to the ip4 and ip6. We only give an 
        // hostname while working with ActiveMQ.
        sort(hostnames.begin(), hostnames.end());
        auto it = unique(hostnames.begin(), hostnames.end());
        hostnames.erase(it, hostnames.end());

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
                                                 << "'(InvalidDestinationException: " << ex.what() << ")." << commit;
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

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BrokerPublisher: Active brokers connections: " << MsgProducers << commit;
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

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "BrokerPublisher: collect_messages: " << messages.size() << " messages to be dispatched." << commit;
    if (messages.size() >= 100000) {
        return;
    }

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
            } catch (...) {
                FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "BrokerPublisher: unknown exception occurred!" << commit;
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
                    if (!MsgProducers.empty()) {
                        monitoring_msg->state = MonitoringMessageCallback::MessageState::sending;
                        sendMessage(*monitoring_msg);
                    }
                    std::advance(msg, 1);
                } else {
                    msg = messages.erase(msg);
                }
                break;
            case MonitoringMessageCallback::MessageState::sending:
                std::advance(msg, 1);
                break;
            case MonitoringMessageCallback::MessageState::delivered:
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Delivered to \"" << monitoring_msg->broker_destination << "\": {"
                                                << monitoring_msg->logging << "}" << commit;
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
                FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Failed to deliver: {" << monitoring_msg->logging << "} to \""
                                                << monitoring_msg->broker_destination << "\" ActiveMQ broker" << commit;
                if (!stop_token.stop_requested()) {
                    monitoring_msg->state = MonitoringMessageCallback::MessageState::ready;
                    if (!MsgProducers.empty()) {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "An other ActiveMQ broker will be used for the dispatch" << commit;
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
            if ((MsgProducers.empty() || time_elapsed.count() >= broker_check_interval) && !stop_token.stop_requested()) {
                if (!refresh_sessions()) {
                    std::condition_variable_any cv;
                    std::mutex mtx;
                    const unsigned int retry_delay = 10;
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unable to connect to the ActiveMQ broker(s) provided by '"
                                                     << brokerConfig.GetBroker() << "' alias." << commit;
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BrokerPublisher: Attempt to re-establish the connections to '"
                                                     << brokerConfig.GetBroker() << "' in " << retry_delay << " seconds." << commit;
                    cv.wait_for(mtx, stop_token, std::chrono::seconds(retry_delay), [] {return false;});
                } else {
                    start_time = std::chrono::steady_clock::now();
                }
            }

            // Get the messages loaded in memory from the MessageLoader thread. Prepare MonitoringMessageCallback's for async dispatch
            if (!stop_token.stop_requested()) {
                BrokerPublisher::collect_messages();
            }

            // Round-robin asyncronous dispatch to ActiveMQ brokers
            BrokerPublisher::dispatch_messages();
            if (MsgProducers.empty() && !stop_token.stop_requested()) {
                FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "No ActiveMQ brokers are available anymore. Trying to reconnect..." << commit;
            }
        } catch (const cms::CMSException &ex) {
            FTS3_COMMON_LOGGER_LOG(ERR, ex.getMessage());
        } catch (const std::exception &ex) {
            FTS3_COMMON_LOGGER_LOG(ERR, ex.what());
        } catch (...) {
            FTS3_COMMON_LOGGER_LOG(CRIT, "Unexpected exception");
        }

        if (stop_token.stop_requested() && messages.empty()) {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Leaving BrokerPublisher Thread" << commit;
            break;
        }
    }
}
