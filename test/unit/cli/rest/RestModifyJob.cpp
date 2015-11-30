/*
 * Copyright (c) CERN 2015
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

#include <boost/property_tree/json_parser.hpp>
#include <boost/test/included/unit_test.hpp>

#include "cli/rest/RestModifyJob.h"
#include "MockHttpRequest.h"

using fts3::cli::RestModifyJob;
namespace pt = boost::property_tree;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(RestModifyJobTest)


BOOST_AUTO_TEST_CASE(ModifyJob)
{
    RestModifyJob modify("abcde-fgehi", 5);

    std::string resource = modify.resource();
    BOOST_CHECK_EQUAL(resource, "/jobs/abcde-fgehi");

    std::stringstream req(modify.body());
    MockHttpRequest http("https://fts3.nowhere.com" + resource, "/etc/grid-security/certificates",
        "/tmp/myproxy.pem", req);

    modify.do_http_action(http);

    BOOST_CHECK_EQUAL(http.method, "POST");

    pt::ptree tree;
    pt::json_parser::read_json(http.reqBody, tree);

    auto params = tree.get_child("params");
    int priority = params.get<int>("priority");
    BOOST_CHECK_EQUAL(priority, 5);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
