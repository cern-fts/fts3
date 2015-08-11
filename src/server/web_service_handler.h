/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include "server_dev.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <string>

extern bool  stopThreads;


FTS3_SERVER_NAMESPACE_START

/** \bried Handle web service events.
 *
 * There are two classes of handling tasks: handling inoming connection, and
 * handling method calls. The basic workflow:
 *
 * - Accept a connection
 * - Schedule mthod handling
 * - Handle the event
 *
 * The event handler should be used as Active Object.
 */
template
<
typename TRAITS
>
class WebServiceHandler : public TRAITS::ActiveObjectType
{
protected:

    using TRAITS::ActiveObjectType::_enqueue;

public:

    /* ---------------------------------------------------------------------- */

    typedef WebServiceHandler <TRAITS> OwnType;

    /* ---------------------------------------------------------------------- */

    /** Constructor. */
    WebServiceHandler
    (
        const std::string& desc = "" /**< Description of this service handler
            (goes to log) */
    ) :
        TRAITS::ActiveObjectType ("WebServiceHandler", desc)
    {}

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~WebServiceHandler()
    {}

    /* ---------------------------------------------------------------------- */

    void listen_p
    (
        const unsigned int port,
        const std::string& ip
    )
    {
        boost::function<void()> op = boost::bind(&WebServiceHandler::_listen_a, this, port, ip);
        this->_enqueue(op);
    }

protected:

    /* ---------------------------------------------------------------------- */

    /** Active counterpart of listen_p (doing the job) */
    void _listen_a
    (
        const unsigned int port,
        const std::string& ip
    )
    {
        typename TRAITS::Acceptor acceptor (port, ip);

        while(stopThreads==false)
            {
                std::shared_ptr<typename TRAITS::Handler> handler = acceptor.accept();

                if (handler.get())
                    {
                        boost::function<void()> op = boost::bind(&WebServiceHandler::_handle_a, this, handler);
                        this->_enqueue(op);
                    }
                else
                    {
                        // if we were not able to accept the connection lets wait for a sec
                        // so we don't loop like crazy in case the system is out of descriptors
                        sleep(1);
                    }

                if (_testHelper.loopOver)
                    {
                        return;
                    }
            }
    }

    /* ---------------------------------------------------------------------- */

    /** Active counterpart of _handle_a (doing the job) */
    void _handle_a
    (
        std::shared_ptr<typename TRAITS::Handler> handler /**< Web service method handler object */
    )
    {
        //assert (handler.get());
        if(handler)
            handler->handle();
    }

    /* ---------------------------------------------------------------------- */
    struct TestHelper
    {
        TestHelper()
            : loopOver (false)
        {}

        bool loopOver;
    }
    _testHelper;
};

FTS3_SERVER_NAMESPACE_END

