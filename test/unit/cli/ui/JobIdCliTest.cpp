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

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>

#include "cli/ui/JobIdCli.h"

using fts3::cli::JobIdCli;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(JobIdCliTest)


BOOST_AUTO_TEST_CASE(JobIdCliTest)
{
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "ID1",
        "ID2",
        "ID3"
    };

    JobIdCli cli;
    cli.parse(argv.size(), (char**)argv.data());
    cli.validate();

    const std::vector<std::string> ids = cli.getJobIds();

    // the vector should have 3 elements
    BOOST_CHECK(ids.size() == 3);
    // check if the values are correct
    BOOST_CHECK(ids[0] == "ID1");
    BOOST_CHECK(ids[1] == "ID2");
    BOOST_CHECK(ids[2] == "ID3");
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
