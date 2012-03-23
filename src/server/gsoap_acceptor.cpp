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

#include "common/logger.h"
#include "common/error.h"
#include "gsoap_acceptor.h"
#include "gsoap_method_handler.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_SERVER_NAMESPACE_START

using namespace FTS3_COMMON_NAMESPACE;

/* -------------------------------------------------------------------------- */

GSoapAcceptor::GSoapAcceptor
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

/* -------------------------------------------------------------------------- */

Pointer<GSoapMethodHandler>::Shared GSoapAcceptor::accept()
{
    SOAP_SOCKET sock = _srv.accept();
    Pointer<GSoapMethodHandler>::Shared handler;

    if (sock >= 0)
    {
        FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New connection, bound to socket " << sock << commit;
        Pointer<transfer::FileTransferSoapBindingService>::Shared service(_srv.copy());
        handler.reset (new GSoapMethodHandler (service));
    }
    else
    {
        FTS3_COMMON_EXCEPTION_LOGERROR (Err_System ("Unable to accept connection request."));
    }

    return handler;
}

FileTransferSoapBindingService GSoapAcceptor::_srv;

FTS3_SERVER_NAMESPACE_END

