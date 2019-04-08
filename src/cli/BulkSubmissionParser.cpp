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

#include "BulkSubmissionParser.h"

#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/tokenizer.hpp>


namespace fts3 {
namespace cli {

const std::set<std::string> BulkSubmissionParser::file_tokens =
    boost::assign::list_of
        ("sources")
        ("destinations")
        ("selection_strategy")
        ("checksum")
        ("checksums")
        ("filesize")
        ("metadata")
        ("activity");

BulkSubmissionParser::BulkSubmissionParser(std::istream &ifs)
{
    try {
        // JSON parsing
        read_json(ifs, pt);

    }
    catch (pt::json_parser_error &ex) {
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
    boost::optional<pt::ptree &> v;
    if (isArray(pt, "Files")) {
        v = pt.get_child_optional("Files");
    }
    else if (isArray(pt, "files")) {
        v = pt.get_child_optional("files");
    }
    else {
        throw cli_exception("There is no array called 'Files' or 'files'");
    }


    pt::ptree &root = v.get();
    for (auto it = root.begin(); it != root.end(); it++) {
        std::pair<std::string, pt::ptree> p = *it;
        pt::ptree &item = p.second;
        validate(item);
        parse_item(item);
    }

    jobParams = pt.get_child_optional("Params");
    if (!jobParams.is_initialized()) {
        jobParams = pt.get_child_optional("params");
    }
}

void BulkSubmissionParser::parse_item(pt::ptree &item)
{

    File file;

    boost::optional<std::string> v_str;
    boost::optional<std::vector<std::string> > v_vec;

    // handle sources

    if (isArray(item, "sources")) {
        // check if 'sources' exists
        v_vec = get<std::vector<std::string> >(item, "sources");
        // if no throw an exception (it is an mandatory field
        if (!v_vec.is_initialized()) throw cli_exception("A file item without 'sources'!");
        file.sources = v_vec.get();

    }
    else {
        // check if 'sources' exists
        v_str = get<std::string>(item, "sources");
        // if no throw an exception (it is an mandatory field
        if (!v_str.is_initialized()) throw cli_exception("A file item without 'sources'!");
        file.sources.push_back(v_str.get());
    }

    // handle destinations

    if (isArray(item, "destinations")) {
        // check if 'destinations' exists
        v_vec = get<std::vector<std::string> >(item, "destinations");
        // if no throw an exception (it is an mandatory field
        if (!v_vec.is_initialized()) throw cli_exception("A file item without 'destinations'!");
        file.destinations = v_vec.get();

    }
    else {
        // check if 'destinations' exists
        v_str = get<std::string>(item, "destinations");
        // if no throw an exception (it is an mandatory field
        if (!v_str.is_initialized()) throw cli_exception("A file item without 'destinations'!");
        file.destinations.push_back(v_str.get());
    }

    // handle selection_strategy

    file.selection_strategy = get<std::string>(item, "selection_strategy");
    if (file.selection_strategy.is_initialized()) {

        std::string selectionStrategy = file.selection_strategy.get();

        if (selectionStrategy != "auto" && selectionStrategy != "orderly") {
            throw cli_exception("'" + selectionStrategy + "' is not a valid selection strategy!");
        }
    }

    // handle checksum 
    // for backward compatibility we accept both checksum and checksums strings
    file.checksum = get<std::string>(item, "checksum");
    if(!file.checksum.is_initialized()) {
        file.checksum = get<std::string>(item, "checksums");
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

void BulkSubmissionParser::validate(pt::ptree &item)
{
    // just validate the main tokens
    pt::ptree::iterator it;
    for (it = item.begin(); it != item.end(); it++) {
        // iterate over the nodes and check if there are in the expexted tokens set
        std::pair<std::string, pt::ptree> p = *it;
        if (!file_tokens.count(p.first)) throw cli_exception("unexpected identifier: " + p.first);
    }
}

bool BulkSubmissionParser::isArray(pt::ptree &item, std::string path)
{
    // get the value for the given path
    boost::optional<pt::ptree &> value = item.get_child_optional(path);
    // check if the value exists
    // if no it's not an array
    if (!value.is_initialized()) return false;
    // the potential array
    pt::ptree &array = value.get();
    // check if the node has a value,
    // accordingly to boost it should be empty if array syntax was used in JSON
    if (!array.get_value<std::string>().empty()) return false;
    // if the checks have been passed successfuly it is an array
    return true;
}

boost::optional<std::string> BulkSubmissionParser::getMetadata(pt::ptree &item)
{
    // get the value for the given path
    boost::optional<pt::ptree &> value = item.get_child_optional("metadata");
    // check if the value exists
    if (!value.is_initialized()) return boost::optional<std::string>();
    // the potential array
    pt::ptree &metadata = value.get();
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

pt::ptree BulkSubmissionParser::getJobParameters()
{
    if (jobParams.is_initialized()) {
        return jobParams.get();
    }
    else {
        return pt::ptree();
    }
}

} /* namespace cli */
} /* namespace fts3 */
