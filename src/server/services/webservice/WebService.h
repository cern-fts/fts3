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
#ifndef WEBSERVICE_H_
#define WEBSERVICE_H_

#include <string>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "common/ThreadPool.h"

#include "GsoapAcceptor.h"
#include "GsoapRequestHandler.h"
#include "../BaseService.h"


namespace fts3 {
namespace server {

/** \brief Handle web service events.
 *
 * There are two classes of handling tasks: handling inoming connection, and
 * handling method calls. The basic workflow:
 *
 * - Accept a connection
 * - Schedule mthod handling
 * - Handle the event
 */
class WebService: public BaseService
{
private:
    int port;
    std::string ip;
    std::shared_ptr<common::ThreadPool<GSoapRequestHandler>> threadPool;

public:

    WebService(int port, const std::string& ip): port(port), ip(ip)
    {
        int threadPoolSize = fts3::config::ServerConfig::instance().get<int>("ThreadNum");
        if (threadPoolSize > 100) {
            threadPoolSize = 100;
        }
        else if (threadPoolSize < 0) {
            threadPoolSize = 2;
        }
        threadPool.reset(new common::ThreadPool<GSoapRequestHandler>(threadPoolSize));
    }

    virtual std::string getServiceName()
    {
        return std::string("WebService");
    }

    virtual void runService()
    {
        GSoapAcceptor acceptor(port, ip);

        while (!boost::this_thread::interruption_requested())
        {
            std::unique_ptr<GSoapRequestHandler> handler = acceptor.accept();

            if (handler.get())
            {
                threadPool->start(handler.release());
            }
            else
            {
                // if we were not able to accept the connection lets wait for a sec
                // so we don't loop like crazy in case the system is out of descriptors
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            }
        }
    }
};

} // end namespace server
} // end namespace fts3

#endif // WEBSERVICE_H_
