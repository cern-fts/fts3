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

#include <fstream>
#include <arpa/inet.h>
#include <netdb.h>
#include "BrokerConfig.h"
#include "config/ServerConfig.h"
#include "common/Logger.h"

namespace po = boost::program_options;

BrokerConfig::BrokerConfig(const std::string &path)
{
    po::options_description opt_desc("Configuration");

    opt_desc.add_options()
    (
        "BROKER_DNS_ALIAS",
        po::value<std::string>()->default_value(""),
        "Broker"
    )
    (
        "STOMP_PORT",
        po::value<std::string>()->default_value("61613"),
        "Broker port number for STOMP"
    )
    (
        "SSL_PORT",
        po::value<std::string>()->default_value("61616"),
        "Broker port number for STOMP over SSL"
    )
    (
        "BROKER_CHECK_INTERVAL",
        po::value<unsigned int>()->default_value(60U),
        "Interval in minutes between two check of the broker connections"
    )
    (
        "START",
        po::value<std::string>()->default_value("transfer.fts_monitoring_start"),
        "Destination for start messages"
    )
    (
        "COMPLETE",
        po::value<std::string>()->default_value("transfer.fts_monitoring_complete"),
        "Destination for completion messages"
    )
    (
        "TTL",
        po::value<int>()->default_value(24),
        "Time to live, in hours"
    )
    (
        "TOPIC",
        po::value<bool>()->default_value(true),
        "Use topics instead of queues"
    )
    (
        "ACTIVE",
        po::value<bool>()->default_value(true),
        "Monitoring active"
    )
    (
        "LOGFILEDIR",
        po::value<std::string>()->default_value("/var/log/fts3/"),
        "Log directory"
    )
    (
        "LOGFILENAME",
        po::value<std::string>()->default_value("msg.log"),
        "Log file name"
    )
    (
        "PUBLISH_FQDN",
        po::value<bool>()->default_value(false),
        "Publish host FQDN on every message"
    )
    (
        "USERNAME",
        po::value<std::string>()->default_value(""),
        "Broker user name"
    )
    (
        "PASSWORD",
        po::value<std::string>()->default_value(""),
        "Broker password"
    )
    (
        "USE_BROKER_CREDENTIALS",
        po::value<bool>()->default_value(false),
        "Send username/password to the broker"
    )
    (
        "STATE",
        po::value<std::string>()->default_value("transfer.fts_monitoring_state"),
        "Destination for state messages"
    )
    (
        "SSL",
        po::value<bool>()->default_value(false),
        "Enable ssl for the messaging"
    )
    (
        "SSL_VERIFY",
        po::value<bool>()->default_value(true),
        "Verify the peer certificate"
    )
    (
        "SSL_ROOT_CA",
        po::value<std::string>()->default_value(""),
        "ROOT CA for the broker"
    )
    (
        "SSL_CLIENT_KEYSTORE",
        po::value<std::string>()->default_value(""),
        "Pem file with client certificate and private key"
    )
    (
        "SSL_CLIENT_KEYSTORE_PASSWORD",
        po::value<std::string>()->default_value(""),
        "Password for the client private key if encrypted"
    )
    (
        "OPTIMIZER",
        po::value<std::string>()->default_value("transfer.fts_monitoring_queue_state"),
        "Destination for optimizer messages"
    )
    ;

    std::ifstream monitoringConfigFile{path};
    if (!monitoringConfigFile.is_open()) {
        FTS3_COMMON_LOGGER_LOG(CRIT, "Unable to open FTS monitoring configuration file: " + path);
        const std::string fallback = "/etc/fts3/fts-msg-monitoring.conf";
        try {
            FTS3_COMMON_LOGGER_LOG(CRIT, "Try to fallback with " + fallback + " configuration file");
            monitoringConfigFile.open(fallback);
        } catch (...) {
            FTS3_COMMON_LOGGER_LOG(CRIT, "Unable to open FTS monitoring configuration file: " + fallback);
            throw;
        }
    }

    if (!monitoringConfigFile.is_open()) {
        FTS3_COMMON_LOGGER_LOG(CRIT, "Unable to open the configuration file.");
        exit(EXIT_FAILURE);
    }

    po::store(po::parse_config_file(monitoringConfigFile, opt_desc, true), vm);

    monitoringConfigFile.close();
}


std::string BrokerConfig::GetLogFilePath() const
{
    auto dir = vm["LOGFILEDIR"].as<std::string>();
    auto file = vm["LOGFILENAME"].as<std::string>();
    return dir + file;
}

std::string BrokerConfig::GetBrokerURI() const
{
    auto broker = GetBroker();
    return GetConnectionString(broker);
}

std::string BrokerConfig::GetConnectionString(const std::string &broker) const
{
    const std::string protocol = UseSSL() ? "ssl" : "tcp";
    const std::string protocol_port = UseSSL() ? GetBrokerStompSslPort() : GetBrokerStompPort();

    std::ostringstream str;
    str << protocol << "://" << broker << ":" << protocol_port<< "?wireFormat=stomp";
    str << "&wireFormat.maxInactivityDuration=2000";
    str << "&maxInactivityDurationInitalDelay=2000";
    if (fts3::config::ServerConfig::instance().get<std::string>("LogLevel") == "DEBUG") {
        str << "&transport.commandTracingEnabled=true";
    }

    return str.str();
}

std::vector<std::string> BrokerConfig::resolve_dns_alias(const std::string &hostname) const
{
    char ipaddr[INET_ADDRSTRLEN];
    struct addrinfo* addresses = NULL;
    const struct addrinfo hints = {
        .ai_flags = AI_CANONNAME,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = 0, .ai_addrlen = 0, .ai_addr = 0, .ai_canonname = 0, .ai_next = 0,
    };
    std::vector<std::string> hostnames;

    int rc = getaddrinfo(hostname.c_str(), NULL, &hints, &addresses);
    if (rc || !addresses) {
        if (addresses) {
            freeaddrinfo(addresses);
        }
        return hostnames;
    }

    for (struct addrinfo *addrP = addresses; addrP != NULL; addrP = addrP->ai_next) {
        char resolved_hostname[NI_MAXHOST];
        inet_ntop(addrP->ai_family, addrP->ai_addr->sa_data, ipaddr, sizeof(ipaddr));
        void *ptr = NULL;
        switch (addrP->ai_family) {
            case AF_INET:
                ptr = &((struct sockaddr_in *) addrP->ai_addr)->sin_addr;
                if (ptr) {
                    inet_ntop(addrP->ai_family, ptr, ipaddr, sizeof(ipaddr));
                }
                break;
            case AF_INET6:
                ptr = &((struct sockaddr_in6 *) addrP->ai_addr)->sin6_addr;
                if (ptr) {
                    inet_ntop(addrP->ai_family, ptr, ipaddr, sizeof(ipaddr));
                }
                break;
        }

        // Reverse DNS. Try to translate the address to a hostname. If successful save hostname for logging
        if (!getnameinfo(addrP->ai_addr, addrP->ai_addrlen, resolved_hostname, sizeof(resolved_hostname), NULL, 0, NI_NAMEREQD)) {
            hostnames.push_back(resolved_hostname);
        }
    }
    freeaddrinfo(addresses);
    return hostnames;
}

std::string BrokerConfig::GetBroker() const
{
    return vm["BROKER_DNS_ALIAS"].as<std::string>();
}

std::string BrokerConfig::GetBrokerStompPort() const
{
    return vm["STOMP_PORT"].as<std::string>();
}

std::string BrokerConfig::GetBrokerStompSslPort() const
{
    return vm["SSL_PORT"].as<std::string>();
}

unsigned int BrokerConfig::GetBrokerCheckInterval() const
{
    return vm["BROKER_CHECK_INTERVAL"].as<unsigned int>();
}

bool BrokerConfig::UseBrokerCredentials() const
{
    return vm["USE_BROKER_CREDENTIALS"].as<bool>();
}


std::string BrokerConfig::GetUserName() const
{
    return vm["USERNAME"].as<std::string>();
}


std::string BrokerConfig::GetPassword() const
{
    return vm["PASSWORD"].as<std::string>();
}


bool BrokerConfig::PublishFQDN() const
{
    return vm["PUBLISH_FQDN"].as<bool>();
}


bool BrokerConfig::UseMsgBroker() const
{
    return vm["ACTIVE"].as<bool>();
}


bool BrokerConfig::UseTopics() const
{
    return vm["TOPIC"].as<bool>();
}


std::string BrokerConfig::GetStartDestination() const
{
    return vm["START"].as<std::string>();
}


std::string BrokerConfig::GetCompleteDestination() const
{
    return vm["COMPLETE"].as<std::string>();
}


std::string BrokerConfig::GetStateDestination() const
{
    return vm["STATE"].as<std::string>();
}


std::string BrokerConfig::GetOptimizerDestination() const
{
    return vm["OPTIMIZER"].as<std::string>();
}


int BrokerConfig::GetTTL() const
{
    return vm["TTL"].as<int>();
}


bool BrokerConfig::UseSSL() const
{
    return vm["SSL"].as<bool>();
}


bool BrokerConfig::SslVerify() const
{
    return vm["SSL_VERIFY"].as<bool>();
}


std::string BrokerConfig::GetRootCA() const
{
    return vm["SSL_ROOT_CA"].as<std::string>();
}


std::string BrokerConfig::GetClientKeyStore() const
{
    return vm["SSL_CLIENT_KEYSTORE"].as<std::string>();
}


std::string BrokerConfig::GetClientKeyStorePassword() const
{
    return vm["SSL_CLIENT_KEYSTORE_PASSWORD"].as<std::string>();
}
