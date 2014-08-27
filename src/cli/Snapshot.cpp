/*
 * Snapshot.cpp
 *
 *  Created on: 27 Aug 2014
 *      Author: simonm
 */

#include "Snapshot.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "unittest/testsuite.h"
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#endif

namespace fts3
{
namespace cli
{

void Snapshot::print(pt::ptree & out) const
{
    out.put("active", active);
    out.put("avg_queued", avg_queued);

    pt::ptree avg_throughput;

    pt::ptree at15;
    at15.put("15", _15);
    avg_throughput.push_back(std::make_pair("", at15));

    pt::ptree at5;
    at5.put("5", _5);
    avg_throughput.push_back(std::make_pair("", at5));

    pt::ptree at30;
    at30.put("30", _30);
    avg_throughput.push_back(std::make_pair("", at30));

    pt::ptree at60;
    at60.put("60", _60);
    avg_throughput.push_back(std::make_pair("", at60));

    out.put_child("avg_throughput", avg_throughput);

    out.put("dest_se", dst_se);
    out.put("failed", failed);
    out.put("finished", finished);

    pt::ptree error;

    pt::ptree count;
    count.put("count", frequent_error.first);
    error.push_back(std::make_pair("", count));

    pt::ptree reason;
    reason.put("reason", frequent_error.second);
    error.push_back(std::make_pair("", reason));

    out.put_child("frequent_error", error);

    out.put("max_active", max_active);
    out.put("source_se", src_se);
    out.put("submitted", submitted);
    out.put("success_ratio", success_ratio);
    out.put("vo_name", vo);
}

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(SnapshotTest)

static std::string const snapshot =
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

static std::string to_string(pt::ptree const & out)
{
    std::stringstream stream;
    pt::write_json(stream, out);
    return stream.str();
}

static std::string get_expected(std::string const & json)
{
    std::stringstream stream;
    // put the JSON message into stream
    stream << json;
    pt::ptree out;
    // create a ptree from the stream
    pt::read_json(stream, out);
    // get the json from the ptree (this ensures white-spaces should be used in the same way as in result)
    return to_string(out);
}

BOOST_FIXTURE_TEST_CASE (Snapshot_print, Snapshot)
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
    BOOST_CHECK_EQUAL(get_expected(snapshot), to_string(result));
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // FTS3_COMPILE_WITH_UNITTESTS

} /* namespace cli */
} /* namespace fts3 */
