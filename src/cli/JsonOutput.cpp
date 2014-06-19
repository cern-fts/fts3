/*
 * JsonOutput.cpp
 *
 *  Created on: 8 May 2014
 *      Author: simonm
 */

#include "JsonOutput.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

namespace fts3
{

namespace cli
{

boost::scoped_ptr<JsonOutput> JsonOutput::instance;

void JsonOutput::create(std::ostream& out)
{
    instance.reset(new JsonOutput(out));
}

void JsonOutput::print(std::string path, std::string msg)
{
    if (instance.get())
        {
            instance->json_out.put(path, msg);
        }
}

void JsonOutput::print(cli_exception const & ex)
{
    if (instance.get())
        {
            instance->json_out.push_back(
                pt::ptree::value_type(
                    ex.json_node(),
                    ex.json_obj()
                )
            );
        }
}

void JsonOutput::printArray(std::string const path, std::map<std::string, std::string> const & object)
{
    printArray(path, to_ptree(object));
}

void JsonOutput::printArray(std::string const path, std::string const value)
{
    pt::ptree item;
    item.put("", value);

    printArray(path, item);
}

void JsonOutput::printArray(std::string const path, pt::ptree const & obj)
{
    if (!instance.get()) return;

    boost::optional<pt::ptree&> child = instance->json_out.get_child_optional(path);

    if (child.is_initialized())
        {
            child.get().push_back(std::make_pair("", obj));
        }
    else
        {
            pt::ptree new_child;
            new_child.push_back(std::make_pair("", obj));
            instance->json_out.put_child(path, new_child);
        }
}

JsonOutput::~JsonOutput()
{
    if (!json_out.empty())
        pt::write_json(out, json_out);
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
