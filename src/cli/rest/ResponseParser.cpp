/*
 * ResponseParser.cpp
 *
 *  Created on: Feb 21, 2014
 *      Author: simonm
 */

#include "ResponseParser.h"

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
            throw ex.message();
        }
}

ResponseParser::~ResponseParser()
{

}

string ResponseParser::get(string path)
{
    return response.get<string>(path);
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
