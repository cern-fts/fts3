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
//#include "ws/evn.h"
//#include "ws/fts.nsmap"
 
#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_SERVER_NAMESPACE_START

using namespace FTS3_COMMON_NAMESPACE;

/* -------------------------------------------------------------------------- */

GSoapAcceptor::GSoapAcceptor
(
    const unsigned int,
    const std::string&
) :
    _isConnectionClosed(true)
{
#if 0
    SOAP_SOCKET sock = _srv.bind(ip.c_str(), port, 0);
    bool isValidSocket = soap_valid_socket(sock);

    if (isValidSocket)
    {
        FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Soap service bound to socket " << sock << commit;
    }
    else
    {
        FTS3_COMMON_EXCEPTION_THROW (Err_System ("Unable to bound to socket."));
    }
#endif
}

/* -------------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST

struct Test_GSoapAcceptor : public GSoapAcceptor
{
    Test_GSoapAcceptor()
        : GSoapAcceptor (Port(), IP())
    {}

    static unsigned int Port()
    {
        return 7777;
    }

    static std::string IP()
    {
        return "localhost";
    }
};

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Server_GSoapAcceptor_constructor, Test_GSoapAcceptor)
{
    BOOST_CHECK(_isConnectionClosed);
    BOOST_CHECK(isConnectionClosed());
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

/* -------------------------------------------------------------------------- */

void GSoapAcceptor::accept() 
{
}

/* -------------------------------------------------------------------------- */

bool GSoapAcceptor::isConnectionClosed() 
{
    return _isConnectionClosed;
}

/* -------------------------------------------------------------------------- */

Pointer<GSoapMethodHandler>::Shared GSoapAcceptor::getHandler()
{
    return Pointer<GSoapMethodHandler>::Shared();
}


/* -------------------------------------------------------------------------- */

bool GSoapAcceptor::isNewConnection()
{
    return false;
}

FTS3_SERVER_NAMESPACE_END

