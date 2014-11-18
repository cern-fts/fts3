/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * BulkSubmissionParser.cpp
 *
 *  Created on: Feb 11, 2013
 *      Author: Michał Simon
 */

#include "BulkSubmissionParser.h"

#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/tokenizer.hpp>

namespace fts3
{
namespace cli
{

using namespace boost::assign;

const std::set<std::string> BulkSubmissionParser::file_tokens =
    list_of
    ("sources")
    ("destinations")
    ("selection_strategy")
    ("checksums")
    ("filesize")
    ("metadata")
    ("activity")
    ;

BulkSubmissionParser::BulkSubmissionParser(std::istream& ifs)
{

    try
        {
            // JSON parsing
            read_json(ifs, pt);

        }
    catch(json_parser_error& ex)
        {
            // handle errors in JSON format
            throw cli_exception(ex.message());
        }

    parse();
}

BulkSubmissionParser::~BulkSubmissionParser()
{

}

void BulkSubmissionParser::parse()
{
    // check if the job is empty
    if (pt.empty()) throw cli_exception("The 'Files' elements of the transfer job are missing!");
    // check if there is more than one job in a single file
    if (pt.size() > 1) throw cli_exception("Too many elements in the bulk submission file!");
    // check if the 'Files' have been defined
    optional<ptree&> v = pt.get_child_optional("Files");
    if (!v.is_initialized()) throw cli_exception("The array of files does not exist!");
    // check if it's an array
    if (!isArray(pt, "Files")) throw cli_exception("The 'Files' element is not an array");
    ptree& root = v.get();
    // iterate over all the file in the job and check their format
    ptree::iterator it;
    for (it = root.begin(); it != root.end(); it++)
        {
            // validate the item in array
            std::pair<std::string, ptree> p = *it;
            ptree& item = p.second;
            validate(item);
            parse_item(item);
        }
}

void BulkSubmissionParser::parse_item(ptree& item)
{

    File file;

    optional<std::string> v_str;
    optional< std::vector<std::string> > v_vec;

    // handle sources

    if (isArray(item, "sources"))
        {
            // check if 'sources' exists
            v_vec = get< std::vector<std::string> >(item, "sources");
            // if no throw an exception (it is an mandatory field
            if (!v_vec.is_initialized()) throw cli_exception("A file item without 'sources'!");
            file.sources = v_vec.get();

        }
    else
        {
            // check if 'sources' exists
            v_str = get<std::string>(item, "sources");
            // if no throw an exception (it is an mandatory field
            if (!v_str.is_initialized()) throw cli_exception("A file item without 'sources'!");
            file.sources.push_back(v_str.get());
        }

    // handle destinations

    if (isArray(item, "destinations"))
        {
            // check if 'destinations' exists
            v_vec = get< std::vector<std::string> >(item, "destinations");
            // if no throw an exception (it is an mandatory field
            if (!v_vec.is_initialized()) throw cli_exception("A file item without 'destinations'!");
            file.destinations = v_vec.get();

        }
    else
        {
            // check if 'destinations' exists
            v_str = get<std::string>(item, "destinations");
            // if no throw an exception (it is an mandatory field
            if (!v_str.is_initialized()) throw cli_exception("A file item without 'destinations'!");
            file.destinations.push_back(v_str.get());
        }

    // handle selection_strategy

    file.selection_strategy = get<std::string>(item, "selection_strategy");
    if (file.selection_strategy.is_initialized())
        {

            std::string selectionStrategy = file.selection_strategy.get();

            if (selectionStrategy != "auto" && selectionStrategy != "orderly")
                {
                    throw cli_exception("'" + selectionStrategy + "' is not a valid selection strategy!");
                }
        }

    // handle checksums

    if (isArray(item, "checksums"))
        {
            v_vec = get< std::vector<std::string> >(item, "checksums");
            // if the checksums value was set ...
            if (v_vec.is_initialized())
                {
                    file.checksums = *v_vec;
                }
        }
    else
        {
            // check if checksum exists
            v_str = get<std::string>(item, "checksums");
            // if yes put it into the vector
            if (v_str.is_initialized())
                {
                    file.checksums.push_back(*v_str) ;
                }
        }



    // handle file size

    file.file_size = get<long>(item, "filesize");

    // handle metadata

    file.metadata = getMetadata(item);

    // handle activity
    file.activity = get<std::string>(item, "activity");

    // put the file into the job element vector
    files.push_back(file);
}

void BulkSubmissionParser::validate(ptree& item)
{
    // just validate the main tokens
    ptree::iterator it;
    for (it = item.begin(); it != item.end(); it++)
        {
            // iterate over the nodes and check if there are in the expexted tokens set
            std::pair<std::string, ptree> p = *it;
            if (!file_tokens.count(p.first)) throw cli_exception("unexpected identifier: " + p.first);
        }
}

bool BulkSubmissionParser::isArray(ptree& item, std::string path)
{
    // get the value for the given path
    optional<ptree&> value = item.get_child_optional(path);
    // check if the value exists
    // if no it's not an array
    if (!value.is_initialized()) return false;
    // the potential array
    ptree& array = value.get();
    // check if the node has a value,
    // accordingly to boost it should be empty if array syntax was used in JSON
    if (!array.get_value<std::string>().empty()) return false;
    // if the checks have been passed successfuly it is an array
    return true;
}

optional<std::string> BulkSubmissionParser::getMetadata(ptree& item)
{
    // get the value for the given path
    optional<ptree&> value = item.get_child_optional("metadata");
    // check if the value exists
    if (!value.is_initialized()) return optional<std::string>();
    // the potential array
    ptree& metadata = value.get();
    // parse the metadata back to JSON

    std::string str = metadata.get_value<std::string>();
    if (!str.empty()) return str;

    std::stringstream ss;
    write_json(ss, metadata);
    // return the string
    return ss.str();
}

std::vector<File> BulkSubmissionParser::getFiles()
{
    return files;
}

} /* namespace cli */
} /* namespace fts3 */
