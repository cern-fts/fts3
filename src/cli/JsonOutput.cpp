/*
 * JsonOutput.cpp
 *
 *  Created on: 8 May 2014
 *      Author: simonm
 */

#include "JsonOutput.h"

#include <sstream>

#include <boost/regex.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

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

void JsonOutput::printArray(std::string const & path, std::map<std::string, std::string> const & object)
{
    printArray(path, to_ptree(object));
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
            // then make sure all numbers and key words like true, false and null,
            // and also empty arrays ('[]') are not between double quotes
            static const boost::regex exp("\"(null|true|false|[]|[0-9]+(\\.[0-9]+)?)\"");
            std::string str = boost::regex_replace(str_out.str(), exp, "$1");
            // and finally print it to the output
            (*out) << str;
        }
}

pt::ptree JsonOutput::to_ptree(std::map<std::string, std::string> const & values)
{

    pt::ptree pt;

    std::map<std::string, std::string>::const_iterator it;
    for (it = values.begin(); it != values.end(); ++it)
        {
            pt.put(it->first, it->second);
        }

    return pt;
}

} /* namespace cli */
} /* namespace fts3 */
