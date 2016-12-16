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

#include "cli/rest/ResponseParser.h"

using fts3::cli::ResponseParser;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(ResponseParserTest)


BOOST_AUTO_TEST_CASE(ResponseParserGet)
{
    std::stringstream resp;

    resp << "{\"job_state\": \"FAILED\"}";

    ResponseParser parser(resp);
    BOOST_CHECK_EQUAL(parser.get("job_state"), "FAILED");
    BOOST_CHECK_THROW(parser.get("job_stateeee"), std::runtime_error);
}


BOOST_AUTO_TEST_CASE(ParseString)
{
    ResponseParser parser("{\"job_state\": \"FAILED\"}");
    BOOST_CHECK_EQUAL(parser.get("job_state"), "FAILED");
    BOOST_CHECK_THROW(parser.get("job_stateeee"), std::runtime_error);
}


BOOST_AUTO_TEST_CASE(ParseJobs)
{
    // Boost parser doesn't like arrays on the top, so need to append
    // a 'fake' root element
    ResponseParser parser(
"{\"jobs\": [\
    {\
        \"job_id\": \"abcdef-abcde-4638-98a7-123456789\",\
        \"job_state\": \"FAILED\",\
        \"user_dn\": \"a/b/c\",\
        \"reason\": \"something went wrong\",\
        \"vo_name\": \"dteam\",\
        \"submit_time\": \"2014-04-15T14:02:50\",\
        \"priority\": 5\
    }\
]}");

    auto jobs = parser.getJobs("jobs");
    BOOST_CHECK_EQUAL(jobs.size(), 1);
    BOOST_CHECK_EQUAL(jobs[0].getStatus(), "FAILED");
}


BOOST_AUTO_TEST_CASE(ParseFiles)
{
    // Boost parser doesn't like arrays on the top, so need to append
    // a 'fake' root element
    ResponseParser parser(
"{\"files\": [{\
    \"retry\": 0,\
    \"file_state\": \"FINISHED\",\
    \"reason\": \"\",\
    \"file_id\": 2047162,\
    \"source_surl\": \"mock://test.cern.ch/suft\",\
    \"dest_surl\": \"mock://aplace.es/fkzj\",\
    \"start_time\": \"2015-11-27T13:45:00\",\
    \"finish_time\": \"2015-11-27T13:49:00\",\
    \"staging_start\": null,\
    \"staging_finished\": null\
    }\
]}");

    auto files = parser.getFiles("files");
    BOOST_CHECK_EQUAL(files.size(), 1);
    BOOST_CHECK_EQUAL(files[0].getState(), "FINISHED");
    BOOST_CHECK_EQUAL(files[0].getFileId(), 2047162);
    BOOST_CHECK_EQUAL(files[0].getSource(), "mock://test.cern.ch/suft");
    BOOST_CHECK_EQUAL(files[0].getDestination(), "mock://aplace.es/fkzj");
}


BOOST_AUTO_TEST_CASE(CountNumberOfStates)
{
    ResponseParser parser(
"{\"files\": [{\
        \"file_state\": \"FINISHED\"\
    },\
    {\
        \"file_state\": \"FINISHED\"\
    },\
    {\
        \"file_state\": \"FAILED\"\
    }\
]}");

    BOOST_CHECK_EQUAL(parser.getNb("files", "FINISHED"), 2);
    BOOST_CHECK_EQUAL(parser.getNb("files", "FAILED"), 1);
    BOOST_CHECK_EQUAL(parser.getNb("files", "SUBMITTED"), 0);
}


BOOST_AUTO_TEST_CASE(ParseSnapshot)
{
    ResponseParser parser(
"{\"snapshot\": [{\
    \"avg_throughput\": {\
        \"60\": 0.52,\
        \"5\": 0.46,\
        \"30\": 0.50,\
        \"15\": 0.47\
    },\
    \"dest_se\": \"srm://ppshead.lcg.triumf.ca\",\
    \"limits\": {},\
    \"success_ratio\": 1.0,\
    \"submitted\": 0,\
    \"failed\": 0,\
    \"finished\": 22,\
    \"frequent_error\": null,\
    \"max_active\": 12,\
    \"avg_queued\": null,\
    \"active\": 0,\
    \"source_se\": \"gsiftp://eosatlassftp.cern.ch\",\
    \"vo_name\": \"atlas\"\
}]}");

    auto snapshot = parser.getSnapshot(true);
    BOOST_CHECK_EQUAL(snapshot.size(), 1);
    BOOST_CHECK_EQUAL(snapshot[0].active, 0);
    BOOST_CHECK_EQUAL(snapshot[0].max_active, 12);
    BOOST_CHECK_EQUAL(snapshot[0].vo, "atlas");
    BOOST_CHECK_EQUAL(snapshot[0].src_se, "gsiftp://eosatlassftp.cern.ch");
    BOOST_CHECK_EQUAL(snapshot[0].dst_se, "srm://ppshead.lcg.triumf.ca");
    BOOST_CHECK_EQUAL(snapshot[0].avg_queued, 0);
    BOOST_CHECK_EQUAL(snapshot[0].success_ratio, 1);
    BOOST_CHECK_EQUAL(snapshot[0].finished, 22);
    BOOST_CHECK_EQUAL(snapshot[0].failed, 0);
    BOOST_CHECK_EQUAL(snapshot[0]._5, 0.46);
    BOOST_CHECK_EQUAL(snapshot[0]._15, 0.47);
    BOOST_CHECK_EQUAL(snapshot[0]._30, 0.50);
    BOOST_CHECK_EQUAL(snapshot[0]._60, 0.52);
}


BOOST_AUTO_TEST_CASE(DetailesFiles)
{
    ResponseParser parser(
"{\"files\": [{\
    \"retry\": 0,\
    \"file_state\": \"FINISHED\",\
    \"reason\": \"\",\
    \"file_id\": 2047162,\
    \"source_surl\": \"mock://test.cern.ch/suft\",\
    \"dest_surl\": \"mock://aplace.es/fkzj\",\
    \"start_time\": \"2015-11-27T13:45:00\",\
    \"finish_time\": \"2015-11-27T13:49:00\",\
    \"staging_start\": null,\
    \"staging_finished\": null,\
    \"job_id\": \"abcdef-ghijk\"\
    }\
]}");

    auto files = parser.getDetailedFiles("files");
    BOOST_CHECK_EQUAL(files.size(), 1);
    BOOST_CHECK_EQUAL(files[0].state, "FINISHED");
    BOOST_CHECK_EQUAL(files[0].fileId, 2047162);
    BOOST_CHECK_EQUAL(files[0].src, "mock://test.cern.ch/suft");
    BOOST_CHECK_EQUAL(files[0].dst, "mock://aplace.es/fkzj");
    BOOST_CHECK_EQUAL(files[0].jobId, "abcdef-ghijk");
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
