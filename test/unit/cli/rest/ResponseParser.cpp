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

#include <boost/test/included/unit_test.hpp>

#include "cli/rest/ResponseParser.h"

#include <sstream>

using fts3::cli::ResponseParser;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(ResponseParserTest)

BOOST_AUTO_TEST_CASE (ResponseParserGet)
{
    std::stringstream resp;

    resp << "{\"job_state\": \"FAILED\"}";

    ResponseParser parser (resp);
    BOOST_CHECK_EQUAL(parser.get("job_state"), "FAILED");
    BOOST_CHECK_THROW(parser.get("job_stateeee"), std::runtime_error);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
