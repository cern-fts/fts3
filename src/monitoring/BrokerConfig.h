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


#ifndef FTS3_BROKERCONFIG_H
#define FTS3_BROKERCONFIG_H

#include <boost/program_options.hpp>


class BrokerConfig {
private:
    boost::program_options::variables_map vm;

public:
    BrokerConfig(const std::string &path);

    /// Get the log full path
    std::string GetLogFilePath() const;

    /// Get the broker URI in a format that is suitable for createCMSConnectionFactory,
    /// depending on the configuration
    std::string GetBrokerURI() const;

    /// Return true if user/password credentials must be used
    bool UseBrokerCredentials() const;

    /// User
    std::string GetUserName() const;

    /// Password
    std::string GetPassword() const;

    /// Return true if the destinations are topics instead of queues
    bool UseTopics() const;

    /// Destination for start messages
    std::string GetStartDestination() const;

    /// Destination for completion messages
    std::string GetCompleteDestination() const;

    /// Destination for queue state messages
    std::string GetStateDestination() const;

    /// Messages time-to-live
    int GetTTL() const;
};


#endif //FTS3_BROKERCONFIG_H
