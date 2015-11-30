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

#include "cli/rest/RestBanning.h"
#include "MockHttpRequest.h"

using fts3::cli::RestBanning;
namespace pt = boost::property_tree;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(RestBanningTest)


BOOST_AUTO_TEST_CASE(StorageBanning)
{
    RestBanning banner("gsiftp://whatnot.com", "dteam", "CANCEL", 100, true, false);

    std::string resource = banner.resource();
    BOOST_CHECK_EQUAL(resource, "/ban/se");

    std::stringstream req(banner.body());
    MockHttpRequest http("https://fts3.nowhere.com" + resource, "/etc/grid-security/certificates",
        "/tmp/myproxy.pem", req);
    banner.do_http_action(http);

    pt::ptree tree;
    pt::json_parser::read_json(http.reqBody, tree);

    std::string storage = tree.get<std::string>("storage");
    std::string vo = tree.get<std::string>("vo_name");
    std::string status = tree.get<std::string>("status");
    int timeout = tree.get<int>("timeout");

    BOOST_CHECK_EQUAL(http.method, "POST");
    BOOST_CHECK_EQUAL(storage, "gsiftp://whatnot.com");
    BOOST_CHECK_EQUAL(vo, "dteam");
    BOOST_CHECK_EQUAL(status, "CANCEL");
    BOOST_CHECK_EQUAL(timeout, 100);
}


BOOST_AUTO_TEST_CASE(StorageUnBanning)
{
    RestBanning banner("gsiftp://whatnot.com", "dteam", std::string(), 0, false, false);

    std::string resource = banner.resource();
    BOOST_CHECK_EQUAL(resource, "/ban/se?storage=gsiftp%3A%2F%2Fwhatnot.com");
    BOOST_CHECK(banner.body().empty());

    std::stringstream req;
    MockHttpRequest http("https://fts3.nowhere.com" + resource, "/etc/grid-security/certificates",
        "/tmp/myproxy.pem", req);
    banner.do_http_action(http);

    BOOST_CHECK_EQUAL(http.method, "DELETE");
    std::string aux;
    http.reqBody >> aux;
    BOOST_CHECK(aux.empty());
}


BOOST_AUTO_TEST_CASE(UserBanning)
{
    RestBanning banner("/DN=someone", "", "", 0, true, true);

    std::string resource = banner.resource();
    BOOST_CHECK_EQUAL(resource, "/ban/dn");

    std::stringstream req(banner.body());
    MockHttpRequest http("https://fts3.nowhere.com" + resource, "/etc/grid-security/certificates",
        "/tmp/myproxy.pem", req);
    banner.do_http_action(http);

    pt::ptree tree;
    pt::json_parser::read_json(http.reqBody, tree);

    std::string dn = tree.get<std::string>("user_dn");

    BOOST_CHECK_EQUAL(http.method, "POST");
    BOOST_CHECK_EQUAL(dn, "/DN=someone");
}


BOOST_AUTO_TEST_CASE(UserUnBanning)
{
    RestBanning banner("/DN=someone", "", "", 0, false, true);

    std::string resource = banner.resource();
    BOOST_CHECK_EQUAL(resource, "/ban/dn?user_dn=%2FDN%3Dsomeone");
    BOOST_CHECK(banner.body().empty());

    std::stringstream req;
    MockHttpRequest http("https://fts3.nowhere.com" + resource, "/etc/grid-security/certificates",
        "/tmp/myproxy.pem", req);
    banner.do_http_action(http);

    BOOST_CHECK_EQUAL(http.method, "DELETE");
    std::string aux;
    http.reqBody >> aux;
    BOOST_CHECK(aux.empty());
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
