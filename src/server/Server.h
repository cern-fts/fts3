/*
 * Copyright (c) CERN 2013-2015
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
#ifndef SERVER_H_
#define SERVER_H_

#include <boost/thread.hpp>

#include "common/Singleton.h"
#include "server/common/BaseService.h"

namespace fts3 {
namespace server {

/// Class representing the FTS3 server logic
class Server: public fts3::common::Singleton<Server>
{
public:
    Server();
    ~Server();

    /// Start the services
    void start();

    /// Wait for all services to finish
    void wait();

    /// Stop the services
    void stop();

private:
    void addService(const std::shared_ptr<BaseService>& service);

    std::string processName{"fts_server"};
    boost::thread_group systemThreads;
    std::vector<std::shared_ptr<BaseService>> services;
};

} // end namespace server
} // end namespace fts3

#endif // SERVER_H_
