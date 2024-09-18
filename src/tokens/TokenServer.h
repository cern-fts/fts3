/*
* Copyright (c) CERN 2024
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

#include <boost/thread.hpp>

#include "common/Singleton.h"
#include "server/common/BaseService.h"

namespace fts3 {
namespace token {

class TokenServer: public fts3::common::Singleton<TokenServer>
{
public:
    TokenServer();
    virtual ~TokenServer();

    /// Start the services
    void start();
    /// Wait for all services to finish
    void stop();
    /// Stop the services
    void wait();

private:
    void addService(const std::shared_ptr<fts3::server::BaseService>& service);

    std::string processName{"fts_token"};
    boost::thread_group systemThreads;
    std::vector<std::shared_ptr<fts3::server::BaseService>> services;
};

} // end namespace token
} // end namespace fts3
