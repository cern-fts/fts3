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
#include "BrokerConfig.h"
#include "config/ServerConfig.h"
#include "common/Logger.h"

namespace po = boost::program_options;


BrokerConfig::BrokerConfig(const std::string &path)
{
    po::options_description opt_desc("Configuration");

    opt_desc.add_options()
    (
        "BROKER",
        po::value<std::string>()->default_value(""),
        "Broker"
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

    std::ifstream in;
    in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        in.open(path);
    } catch (const std::system_error& ex) {
        FTS3_COMMON_LOGGER_LOG(DEBUG, "Could not open " + path);
        throw;
    } catch (...) {
        FTS3_COMMON_LOGGER_LOG(DEBUG, "PROCESS_ERROR Unknown exception");
    }
    in.exceptions(std::ifstream::goodbit);

    po::store(po::parse_config_file(in, opt_desc, true), vm);
}


std::string BrokerConfig::GetLogFilePath() const
{
    auto dir = vm["LOGFILEDIR"].as<std::string>();
    auto file = vm["LOGFILENAME"].as<std::string>();
    return dir + file;
}

std::string BrokerConfig::GetBrokerURI() const
{
    auto broker = vm["BROKER"].as<std::string>();
    std::string proto = "tcp";
    if (UseSSL()) {
        proto = "ssl";
    }

    std::ostringstream str;
    str << proto << "://" << broker << "?wireFormat=stomp&soKeepAlive=true&wireFormat.MaxInactivityDuration=-1";

    if (fts3::config::ServerConfig::instance().get<std::string>("LogLevel") == "DEBUG") {
        str << "&transport.commandTracingEnabled=true";
    }

    return str.str();
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
