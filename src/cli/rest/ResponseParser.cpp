/*
 * ResponseParser.cpp
 *
 *  Created on: Feb 21, 2014
 *      Author: simonm
 */

#include "ResponseParser.h"

#include <boost/property_tree/json_parser.hpp>

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

