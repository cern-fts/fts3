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
#include <boost/property_tree/json_parser.hpp>

#include "cli/Snapshot.h"

using fts3::cli::Snapshot;
namespace pt = boost::property_tree;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(SnapshotTest)


static const std::string SNAPSHOT =
    "{"
    "   \"active\": 10,"
    "   \"avg_queued\": 100,"
    "   \"avg_throughput\": [ {\"15\": 15.1}, {\"5\": 5.3}, {\"30\": 30.2}, {\"60\": 60.4} ],"
    "   \"dest_se\": \"srm://dst.desy.de\","
    "   \"failed\": 5,"
    "   \"finished\": 10,"
    "   \"frequent_error\": [ {\"count\": 3}, {\"reason\": \"some error string\"} ],"
    "   \"max_active\": 20,"
    "   \"source_se\": \"srm://src.cern.ch\","
    "   \"submitted\": 30,"
    "   \"success_ratio\": 0.75,"
    "   \"vo_name\": \"dteam\""
    "}"
;


static std::string toString(const pt::ptree &out)
{
    std::stringstream stream;
    pt::write_json(stream, out);
    return stream.str();
}


static std::string getExpected(const std::string &json)
{
    std::stringstream stream;
    // put the JSON message into stream
    stream << json;
    pt::ptree out;
    // create a ptree from the stream
    pt::read_json(stream, out);
    // get the json from the ptree (this ensures white-spaces should be used in the same way as in result)
    return toString(out);
}


BOOST_FIXTURE_TEST_CASE (SnapshotPrint, Snapshot)
{
    // prepare snapshot instance
    vo = "dteam";
    src_se = "srm://src.cern.ch";
    dst_se = "srm://dst.desy.de";
    active = 10;
    max_active = 20;
    failed = 5;
    finished = 10;
    submitted = 30;
    avg_queued = 100;
    _15 = 15.1;
    _30 = 30.2;
    _5  = 5.3;
    _60 = 60.4;
    success_ratio = 0.75;
    frequent_error = std::make_pair(3, "some error string");
    // print it to string
    pt::ptree result;
    print(result);
    // check if the result matches the expected one
    BOOST_CHECK_EQUAL(getExpected(SNAPSHOT), toString(result));
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
