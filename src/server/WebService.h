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

#include "gsoap_acceptor.h"
#include "gsoap_request_handler.h"


extern bool  stopThreads;


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
class WebService
{
private:
    std::shared_ptr<common::ThreadPool<GSoapRequestHandler>> threadPool;

public:

    WebService()
    {
        size_t threadPoolSize = fts3::config::theServerConfig().get<size_t> ("ThreadNum");
        threadPool.reset(new common::ThreadPool<GSoapRequestHandler>(threadPoolSize));
    }

    void listen(unsigned int port, const std::string& ip)
    {
        GSoapAcceptor acceptor(port, ip);

        while (stopThreads == false)
        {
            std::shared_ptr<GSoapRequestHandler> handler = acceptor.accept();

            if (handler.get())
            {
                threadPool->start(handler.get());
                handler.reset();
            }
            else
            {
                // if we were not able to accept the connection lets wait for a sec
                // so we don't loop like crazy in case the system is out of descriptors
                sleep(1);
            }
        }
    }
};

} // end namespace server
} // end namespace fts3

#endif // WEBSERVICE_H_
