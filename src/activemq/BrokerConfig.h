/*
 * Copyright (c) CERN 2025
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

#include <boost/program_options.hpp>


class BrokerConfig {
private:
    boost::program_options::variables_map vm;

public:
    BrokerConfig(const std::string &path);

    /// Get the log full path
    std::string GetLogFilePath() const;

    /// Get the broker host
    std::string GetBroker() const;

    /// Get the broker URI in a format that is suitable for createCMSConnectionFactory,
    /// depending on the configuration
    std::string GetBrokerURI() const;

    /// Get the STOMP port used by the Broker
    std::string GetBrokerStompPort() const;

    /// Get the STOMP/SSL port used by the Broker
    std::string GetBrokerStompSslPort() const;

    /// Get the connection string in a format that is suitable for createCMSConnectionFactory,
    /// depending on the provided broker hostname
    std::string GetConnectionString(const std::string &broker) const;

    unsigned int GetMessageQueueCapacity() const;

    unsigned int GetMessageQueueBatchSize() const;

    unsigned int GetBrokerCheckInterval() const;

    /// Return true if user/password credentials must be used
    bool UseBrokerCredentials() const;

    /// User
    std::string GetUserName() const;

    /// Password
    std::string GetPassword() const;

    /// Publish FQDN flag
    bool PublishFQDN() const;

    /// Return true if the destinations are topics instead of queues
    bool UseTopics() const;

    /// Destination for start messages
    std::string GetStartDestination() const;

    /// Destination for completion messages
    std::string GetCompleteDestination() const;

    /// Destination for queue state messages
    std::string GetStateDestination() const;

    /// Destination for optimizer updates
    std::string GetOptimizerDestination() const;

    /// Resolve broker alias
    std::vector<std::string> resolve_dns_alias(const std::string &hostname) const;

    /// Messages time-to-live, in hours
    int GetTTL() const;

    /// If true, enable SSL for the producer
    bool UseSSL() const;

    /// If true, use the messaging broker
    bool UseMsgBroker() const;

    /// If true, verify broker certificate
    bool SslVerify() const;

    /// ROOT CA for the broker
    std::string GetRootCA() const;

    /// Bundle with the client cert and private key
    std::string GetClientKeyStore() const;

    /// Password for the client cert and private key
    std::string GetClientKeyStorePassword() const;
};
