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

#include "web_service_handler.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "unittest/testsuite.h"
#include <boost/assign/list_of.hpp>
#endif // FTS3_COMPILE_WITH_UNITTESTS

using namespace fts3::server;

#ifdef FTS3_COMPILE_WITH_UNITTEST

/* -------------------------------------------------------------------------- */

struct Test_ActiveObject
{
    Test_ActiveObject
    (
        const std::string& label,
        const std::string& desc
    ) :
        Label (label),
        Desc (desc),
        Enqueued (false)
    {}

    template <class OP>
    void _enqueue (OP&)
    {
        Enqueued = true;
    }

    std::string Label;
    std::string Desc;
    bool Enqueued;
};

/* -------------------------------------------------------------------------- */

struct Test_Handler
{
    Test_Handler()
        : Handled(false)
    {}


    void handle()
    {
        Handled = true;
    }

    bool Handled;
};

/* -------------------------------------------------------------------------- */

struct Test_Acceptor
{
    Test_Acceptor
    (
        const unsigned int port,
        const std::string& ip
    )
    {
        Port = port;
        IP = ip;
        Accepted = false;
    }

    Pointer<Test_Handler>::Shared accept ()

    {
        Accepted = true;
        return Pointer<Test_Handler>::Shared (new Test_Handler);
    }

    static bool Accepted;
    static unsigned int Port;
    static std::string IP;
};

bool Test_Acceptor::Accepted = false;
unsigned int Test_Acceptor::Port = 0;
std::string Test_Acceptor::IP;

/* -------------------------------------------------------------------------- */

struct Test_Traits
{
    typedef Test_ActiveObject ActiveObjectType;
    typedef Test_Acceptor Acceptor;
    typedef Test_Handler Handler;
};

/* -------------------------------------------------------------------------- */

struct Test_WebServiceHandler :
    public WebServiceHandler <Test_Traits>
{
    Test_WebServiceHandler()
        : OwnType (Description())
    {
        _testHelper.loopOver = true ;
    }

    static const std::string& Description()
    {
        static std::string desc("desc");
        return desc;
    }

    void testListen()
    {
        _listen_a (Port(), IP());
    }

    static unsigned int Port()
    {
        return 8080;
    }

    static std::string IP()
    {
        return "localhost";
    }
};

/* -------------------------------------------------------------------------- */

/** Test if base constructors have been called */
BOOST_FIXTURE_TEST_CASE (Server_WebServiceHandler_Constructor, Test_WebServiceHandler)
{
    BOOST_CHECK_EQUAL (Label, "WebServiceHandler");
    BOOST_CHECK_EQUAL (Desc, Description());
}

/* -------------------------------------------------------------------------- */

/** Test is port and IP are passed to Acceptor */
BOOST_FIXTURE_TEST_CASE (Server_WebServiceHandler_port_ip, Test_WebServiceHandler)
{
    testListen();
    BOOST_CHECK_EQUAL (Test_Acceptor::Port, Port());
    BOOST_CHECK_EQUAL (Test_Acceptor::IP, IP());
}
#endif // FTS3_COMPILE_WITH_UNITTESTS


