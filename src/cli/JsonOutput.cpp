/*
 * JsonOutput.cpp
 *
 *  Created on: 8 May 2014
 *      Author: simonm
 */

#include "JsonOutput.h"

#include <sstream>

#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

#include <boost/regex.hpp>

namespace fts3
{

namespace cli
{

void JsonOutput::print(std::string const & path, std::string const & msg)
{
    json_out.put(path, msg);
}

void JsonOutput::print(cli_exception const & ex)
{
    json_out.push_back(
            pt::ptree::value_type(ex.json_node(), ex.json_obj())
        );
}

void JsonOutput::print(std::exception const & ex)
{
    print("error", ex.what());
}

void JsonOutput::printArray(std::string const & path, std::string const & value)
{
    pt::ptree item;
    item.put("", value);

    printArray(path, item);
}

void JsonOutput::printArray(std::string const & path, pt::ptree const & obj)
{
    boost::optional<pt::ptree&> child = json_out.get_child_optional(path);

    if (child.is_initialized())
        {
            child.get().push_back(std::make_pair("", obj));
        }
    else
        {
            pt::ptree new_child;
            new_child.push_back(std::make_pair("", obj));
            json_out.put_child(path, new_child);
        }
}

JsonOutput::~JsonOutput()
{
    if (!json_out.empty())
        {
            // first write the output to a 'stringstream'
            std::stringstream str_out;
            pt::write_json(str_out, json_out);
            // then make sure symbols like null and true/false are not in double quotes
            static const boost::regex exp(":\\s*\"(null|true|false|\\[\\]|[0-9]+(\\.[0-9]+)?)\"");
            (*out) << boost::regex_replace(str_out.str(), exp, ": $1");
            // the use of regex should be reconsidered because it exposes us to 'static deinitialization order fiasco' !!!
        }
}

} /* namespace cli */
} /* namespace fts3 */
