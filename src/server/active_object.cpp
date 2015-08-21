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

#include "active_object.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

using namespace fts3::server;

#ifdef FTS3_COMPILE_WITH_UNITTEST

/* -------------------------------------------------------------------------- */

struct Test_ExecutionPolicy
{

};

/* -------------------------------------------------------------------------- */

struct Test_Tracer
{

};

/* -------------------------------------------------------------------------- */

struct Test_ActiveObject_Default_Constructor
    : public ActiveObject<Test_ExecutionPolicy, Test_Tracer>
{
    Test_ActiveObject_Default_Constructor()
        : BaseType()
    {}
};

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE
(
    Server_ActiveObject_Default_Constructor,
    Test_ActiveObject_Default_Constructor
)
{
    BOOST_CHECK_EQUAL (_runningMethods, 0);
}

/* -------------------------------------------------------------------------- */

struct Test_ActiveObject_Constructor_WithTracer
    : public ActiveObject<Test_ExecutionPolicy, int>
{
    static int Value()
    {
        return 3;
    }

    Test_ActiveObject_Constructor_WithTracer()
        : BaseType(Value())
    {}
};

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE
(
    Server_ActiveObject_Default_WithTracer,
    Test_ActiveObject_Constructor_WithTracer
)
{
    BOOST_CHECK_EQUAL (_runningMethods, 0);
    // This tests it _tracer constructor was called. No other semantics, any copiable
    // object will do here.
    BOOST_CHECK_EQUAL (_tracer, Test_ActiveObject_Constructor_WithTracer::Value());
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

