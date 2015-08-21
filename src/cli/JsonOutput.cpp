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

#include "JsonOutput.h"

#include <sstream>

#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

#include <boost/regex.hpp>

namespace fts3
{

namespace cli
{


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
