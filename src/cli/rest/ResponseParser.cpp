/*
 * ResponseParser.cpp
 *
 *  Created on: Feb 21, 2014
 *      Author: simonm
 */

#include "ResponseParser.h"

#include "exception/cli_exception.h"

#include <boost/property_tree/json_parser.hpp>

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "unittest/testsuite.h"
#include <sstream>
#endif

using namespace fts3::cli;

ResponseParser::ResponseParser(istream& stream)
{
    try
        {
            // parse
            read_json(stream, response);
        }
    catch(json_parser_error& ex)
        {
            throw cli_exception(ex.message());
        }
}

ResponseParser::~ResponseParser()
{

}

string ResponseParser::get(string const & path) const
{
    return response.get<string>(path);
}

vector<JobStatus> ResponseParser::getJobs(string const & path) const
{
	ptree const & jobs = response.get_child(path);

	vector<JobStatus> ret;
	ptree::const_iterator it;

	for (it = jobs.begin(); it != jobs.end(); ++it)
	{
		JobStatus j;
		j.clientDn = it->second.get<string>("user_dn");
		j.jobId = it->second.get<string>("job_id");
		j.jobStatus = it->second.get<string>("job_state");
		j.numFiles = -1;
		j.priority = it->second.get<int>("priority");
		j.reason = it->second.get<string>("reason");
		j.submitTime = it->second.get<long int>("submit_time");
		j.voName = it->second.get<string>("vo_name");
		ret.push_back(j);
	}

	return ret;
}

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(ResponseParserTest)

BOOST_AUTO_TEST_CASE (ResponseParser_get)
{
    stringstream resp;

    resp << "{\"job_state\": \"FAILED\"}";

    ResponseParser parser (resp);
    BOOST_CHECK_EQUAL(parser.get("job_state"), "FAILED");
    BOOST_CHECK_THROW(parser.get("job_stateeee"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // FTS3_COMPILE_WITH_UNITTESTS
