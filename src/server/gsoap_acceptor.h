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
#include "common/pointers.h"
#include "common/error.h"
#include "common/logger.h"
#include "gsoap_method_handler.h"


FTS3_SERVER_NAMESPACE_START

using namespace FTS3_COMMON_NAMESPACE;

template <class SRV>
class GSoapAcceptor
{
public:
    GSoapAcceptor
    (
        const unsigned int port,
        const std::string& ip
    )
    {
        SOAP_SOCKET sock = _srv.bind (ip.c_str(), port, 0);

        if (sock >= 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Soap service bound to socket " << sock << commit;
        }
        else
        {
            FTS3_COMMON_EXCEPTION_THROW (Err_System ("Unable to bound to socket."));
        }
    }

    boost::shared_ptr< GSoapMethodHandler<SRV> > accept()
    {
        SOAP_SOCKET sock = _srv.accept();
        boost::shared_ptr< GSoapMethodHandler<SRV> > handler;

        if (sock >= 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New connection, bound to socket " << sock << commit;
            boost::shared_ptr<SRV> service(_srv.copy());
            handler.reset (new GSoapMethodHandler<SRV> (service));
        }
        else
        {
            FTS3_COMMON_EXCEPTION_LOGERROR (Err_System ("Unable to accept connection request."));
        }

        return handler;
    }

protected:

    static SRV _srv;
};

template <class SRV>
SRV GSoapAcceptor<SRV>::_srv;

FTS3_SERVER_NAMESPACE_END

