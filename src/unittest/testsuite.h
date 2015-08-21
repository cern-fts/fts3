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

/** \file testsuite.h FTS3 unit test suite interface. Include this file
 * in each component test suites. */

#pragma once

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

/** An always true predicate, for BOOST_CHECK_EXCEPTION */
template <class T>
bool fts3_unittest_always_true (const T&)
{
    return true;
}

