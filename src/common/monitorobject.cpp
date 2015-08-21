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

/** \file monitorobject.cpp Implementation of MonitorObject class. */


#include "monitorobject.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

#include "unittest/testsuite.h"

using namespace FTS3_COMMON_NAMESPACE;

// Reflects how to use MonitorObject: Derive from it.
struct MonitorObject_Test : public MonitorObject {};

/* -------------------------------------------------------------------------- */
BOOST_AUTO_TEST_SUITE( common )
BOOST_FIXTURE_TEST_SUITE (MonitorObjectTest, MonitorObject_Test)

BOOST_AUTO_TEST_CASE (test)
{
    FTS3_COMMON_MONITOR_START_CRITICAL
    BOOST_TEST_MESSAGE ("Critical section started.");
    FTS3_COMMON_MONITOR_END_CRITICAL
    BOOST_TEST_MESSAGE ("Critical section finished.");
    FTS3_COMMON_MONITOR_START_STATIC_CRITICAL
    BOOST_TEST_MESSAGE ("Static critical section started.");
    FTS3_COMMON_MONITOR_END_CRITICAL
    BOOST_TEST_MESSAGE ("Static critical section finished.");
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS

