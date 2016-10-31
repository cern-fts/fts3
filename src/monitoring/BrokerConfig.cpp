/*
 * Copyright (c) CERN 2016
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

#include "BrokerConfig.h"
#include <fstream>

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
        "FQDN",
        po::value<std::string>()->default_value(""),
        "Obsolete"
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
    ;

    std::ifstream in(path.c_str());
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
    return "tcp://" + broker + "?wireFormat=stomp&soKeepAlive=true&wireFormat.MaxInactivityDuration=-1";
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


int BrokerConfig::GetTTL() const
{
    return vm["TTL"].as<int>();
}
