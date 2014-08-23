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

ResponseParser::ResponseParser(std::istream& stream)
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

std::string ResponseParser::get(std::string const & path) const
{
    return response.get<std::string>(path);
}

std::vector<JobStatus> ResponseParser::getJobs(std::string const & path) const
{
    ptree const & jobs = response.get_child(path);

    std::vector<JobStatus> ret;
    ptree::const_iterator it;

    for (it = jobs.begin(); it != jobs.end(); ++it)
        {
            JobStatus j(
                    it->second.get<std::string>("job_id"),
                    it->second.get<std::string>("job_state"),
                    it->second.get<std::string>("user_dn"),
                    it->second.get<std::string>("reason"),
                    it->second.get<std::string>("vo_name"),
                    it->second.get<std::string>("submit_time"),
                    -1,
                    it->second.get<int>("priority")
                );

            ret.push_back(j);
        }

    return ret;
}

std::vector<FileInfo> ResponseParser::getFiles(std::string const & path) const
{
    ptree const & files = response.get_child(path);

    std::vector<FileInfo> ret;
    ptree::const_iterator it;

    for (it = files.begin(); it != files.end(); ++it)
        {
            ret.push_back(FileInfo(it->second));
        }

    return ret;
}

int ResponseParser::getNb(std::string const & path, std::string const & state) const
{
    ptree const & files = response.get_child(path);

    int ret = 0;
    ptree::const_iterator it;

    for (it = files.begin(); it != files.end(); ++it)
        {
            if (it->second.get<std::string>("file_state") == state) ++ret;
        }

    return ret;
}

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(ResponseParserTest)

BOOST_AUTO_TEST_CASE (ResponseParser_get)
{
    std::stringstream resp;

    resp << "{\"job_state\": \"FAILED\"}";

    ResponseParser parser (resp);
    BOOST_CHECK_EQUAL(parser.get("job_state"), "FAILED");
    BOOST_CHECK_THROW(parser.get("job_stateeee"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // FTS3_COMPILE_WITH_UNITTESTS
