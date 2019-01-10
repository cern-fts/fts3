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

#include "RestSubmission.h"

#include "JobParameterHandler.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>

#include "../exception/cli_exception.h"


namespace fts3
{
namespace cli
{


void RestSubmission::to_array(std::vector<std::string> const & v, pt::ptree & t)
{
    std::vector<std::string>::const_iterator it;
    for (it = v.begin(); it != v.end(); ++it)
        {
            t.push_back(std::make_pair("", pt::ptree(*it)));
        }
}

void RestSubmission::to_output(std::ostream & os, pt::ptree const & root)
{
    std::stringstream str_out;
    pt::write_json(str_out, root);

    os << strip_values(str_out.str());
}

void RestSubmission::strip_values(std::string & json, std::string const & token)
{
    static std::string const quote = "\"";

    std::string const wanted(quote + token + quote);
    std::string::size_type pos = 0;

    while ( (pos = json.find(wanted, pos)) != std::string::npos)
        {
            // shift the position by the size of the wanted string
            pos += wanted.size();
            // get the substring after the wanted string
            std::string sub = json.substr(pos);
            // after trimming the right substring
            // the first character should be a ':'
            boost::algorithm::trim(sub);
            if (sub[0] != ':') continue;
            // then remove the ':' and following white-spaces
            sub = sub.substr(1);
            boost::algorithm::trim(sub);
            // now the first character should be a quote
            if (sub[0] != '"') continue;
            // now find the closing quote
            std::string::size_type end = sub.find("\"", 1);
            if (end == std::string::npos) continue;
            // get the current value
            std::string value = sub.substr(0, end + 1);
            pos = json.find(value, pos);
            // and replace the current value with a one strip of the quotes
            json.replace(pos, value.size(), value.substr(1, end - 1));
        }
}

std::string RestSubmission::strip_values(std::string const & json)
{
    static std::string tokens[] =
    {"filesize",  "reuse", "bring_online", "copy_pin_lifetime", "overwrite", "multihop", "retry", "timeout"};
    static int const size = 8;

    std::string ret = json;
    for (int index = 0; index < size; ++index)
        {
            strip_values(ret, tokens[index]);
        }

    return ret;
}

std::ostream& operator<<(std::ostream& os, RestSubmission const & me)
{
    pt::ptree root, files, params;
    // Preset with the bulk values
    params = me.extra;

    // prepare the files array
    std::vector<File>::const_iterator it;
    for (it = me.files.begin(); it != me.files.end(); ++it)
        {
            pt::ptree element, sources, destinations;
            // add sources
            RestSubmission::to_array(it->sources, sources);
            element.push_back(std::make_pair("sources", sources));
            // add destinations
            RestSubmission::to_array(it->destinations, destinations);
            element.push_back(std::make_pair("destinations", destinations));
            // add metadata if provided
            if (it->metadata) {
                std::stringstream iostr;
                iostr << *it->metadata;

                pt::ptree file_metadata;
                try {
                    pt::read_json(iostr, file_metadata);
                    element.push_back(std::make_pair("metadata", file_metadata));
                }
                catch (const pt::json_parser::json_parser_error&) {
                    element.put("metadata", *it->metadata);
                }
            }
            // add file size if provided
            if (it->file_size)
                element.put("filesize", boost::lexical_cast<std::string>(*it->file_size));
            // add checksum if provided
            if (it->checksum)
                element.put("checksum", it->checksum.get());
            // add activity
            if (it->activity)
                element.put("activity", it->activity.get());
            // add the element to files array
            files.push_back(std::make_pair("", element));
        }
    // add files to the root
    root.push_back(std::make_pair("files", files));

    // prepare parameters
    if (me.parameters.find(JobParameterHandler::CHECKSUM_METHOD) != me.parameters.end())
        {
            params.put("verify_checksum", true);
        }

    if (me.parameters.find(JobParameterHandler::REUSE) != me.parameters.end())
        {
            params.put("reuse", true);
        }
    std::map<std::string, std::string>::const_iterator param_itr = me.parameters.find(JobParameterHandler::SPACETOKEN);
    if (param_itr != me.parameters.end())
        {
            params.put("spacetoken", param_itr->second);
        }
    param_itr = me.parameters.find(JobParameterHandler::CHECKSUM_MODE);
    if (param_itr != me.parameters.end())
        {
            params.put("verify_checksum", param_itr->second);
        }
    param_itr = me.parameters.find(JobParameterHandler::BRING_ONLINE);
    if (param_itr != me.parameters.end() && param_itr->second != "-1")
        {
            params.put("bring_online", param_itr->second);
        }
    param_itr = me.parameters.find(JobParameterHandler::COPY_PIN_LIFETIME);
    if (param_itr != me.parameters.end() && param_itr->second != "-1")
        {
            params.put("copy_pin_lifetime", param_itr->second);
        }
    param_itr = me.parameters.find(JobParameterHandler::TIMEOUT);
    if (param_itr != me.parameters.end() && param_itr->second != "-1")
       {
            params.put("timeout", param_itr->second);
       }
    param_itr = me.parameters.find(JobParameterHandler::JOB_METADATA);
    if (param_itr != me.parameters.end())
        {
            std::stringstream iostr;
            iostr << param_itr->second;

            pt::ptree job_metadata;
            try {
                pt::read_json(iostr, job_metadata);
                params.push_back(std::make_pair("job_metadata", job_metadata));
            }
            catch (const pt::json_parser::json_parser_error&) {
                params.put("job_metadata", param_itr->second);
            }
        }
    param_itr = me.parameters.find(JobParameterHandler::SPACETOKEN_SOURCE);
    if (param_itr != me.parameters.end())
        {
            params.put("source_spacetoken", param_itr->second);
        }
    param_itr = me.parameters.find(JobParameterHandler::OVERWRITEFLAG);
    if (param_itr != me.parameters.end())
        {
            params.put("overwrite", true);
        }
    param_itr = me.parameters.find(JobParameterHandler::GRIDFTP);
    if (param_itr != me.parameters.end())
        {
            params.put("gridftp", param_itr->second);
        }
    param_itr = me.parameters.find(JobParameterHandler::MULTIHOP);
    if (param_itr != me.parameters.end())
        {
            params.put("multihop", true);
        }
    param_itr = me.parameters.find(JobParameterHandler::RETRY);
    if (param_itr != me.parameters.end())
        {
            params.put("retry", param_itr->second);
        }
    param_itr = me.parameters.find(JobParameterHandler::STRICT_COPY);
    if (param_itr != me.parameters.end())
        {
            params.put("strict_copy", true);
        }
    param_itr = me.parameters.find(JobParameterHandler::NOSTREAMS);
    if (param_itr != me.parameters.end())
        {
            params.put("nostreams", param_itr->second);
        }


    // add params to root
    if (!params.empty())
        root.push_back(std::make_pair("params", params));

    RestSubmission::to_output(os, root);

    return os;
}

} /* namespace cli */
} /* namespace fts3 */
