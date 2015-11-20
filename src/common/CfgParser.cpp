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

#include "CfgParser.h"

#include <sstream>
#include <algorithm>

#include <boost/assign.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>


namespace fts3
{
namespace common
{

using namespace boost::assign;


const std::map<std::string, std::set <std::string> > CfgParser::standaloneSeCfgTokens = CfgParser::initStandaloneSeCfgTokens();
const std::map<std::string, std::set <std::string> > CfgParser::standaloneGrCfgTokens = CfgParser::initStandaloneGrCfgTokens();
const std::map<std::string, std::set <std::string> > CfgParser::sePairCfgTokens = CfgParser::initSePairCfgTokens();
const std::map<std::string, std::set <std::string> > CfgParser::grPairCfgTokens = CfgParser::initGrPairCfgTokens();
const std::map<std::string, std::set <std::string> > CfgParser::shareOnlyCfgTokens = CfgParser::initShareOnlyCfgTokens();
const std::map<std::string, std::set <std::string> > CfgParser::activityShareCfgTokens = CfgParser::initActivityShareCfgTokens();

const std::string CfgParser::auto_value = "auto";

const std::set<std::string> CfgParser::allTokens =
    list_of
    ("se")
    ("group")
    ("members")
    ("active")
    ("in")
    ("out")
    ("share")
    ("protocol")
    ("symbolic_name")
    ("source_se")
    ("destination_se")
    ("source_group")
    ("destination_group")
    ("vo")
    ;

const std::map< std::string, std::set <std::string> > CfgParser::initStandaloneSeCfgTokens()
{
    std::set<std::string> root = list_of
                                 ("se")
                                 ("active")
                                 ("in")
                                 ("out")
                                 ;

    std::set<std::string> cfg = list_of
                                ("share")
                                ("protocol")
                                ;

    return map_list_of
           (std::string(), root)
           ("in", cfg)
           ("out", cfg)
           ;
}

const std::map< std::string, std::set <std::string> > CfgParser::initStandaloneGrCfgTokens()
{
    std::set<std::string> root = list_of
                                 ("group")
                                 ("members")
                                 ("active")
                                 ("in")
                                 ("out")
                                 ;

    std::set<std::string> cfg = list_of
                                ("share")
                                ("protocol")
                                ;

    return map_list_of
           (std::string(), root)
           ("in", cfg)
           ("out", cfg)
           ;
}

const std::map< std::string, std::set <std::string> > CfgParser::initSePairCfgTokens()
{
    std::set<std::string> root = list_of
                                 ("symbolic_name")
                                 ("active")
                                 ("source_se")
                                 ("destination_se")
                                 ("share")
                                 ("protocol")
                                 ;

    return map_list_of
           (std::string(), root)
           ;
}

const std::map< std::string, std::set <std::string> > CfgParser::initGrPairCfgTokens()
{
    std::set<std::string> root = list_of
                                 ("symbolic_name")
                                 ("active")
                                 ("source_group")
                                 ("destination_group")
                                 ("share")
                                 ("protocol")
                                 ;

    return map_list_of
           (std::string(), root)
           ;
}

const std::map<std::string, std::set <std::string> > CfgParser::initShareOnlyCfgTokens()
{
    std::set<std::string> root = list_of
                                 ("se")
                                 ("active")
                                 ("in")
                                 ("out")
                                 ;

    return map_list_of
           (std::string(), root)
           ;
}

const std::map<std::string, std::set <std::string> > CfgParser::initActivityShareCfgTokens()
{
    std::set<std::string> root = list_of
                                 ("vo")
                                 ("active")
                                 ("share")
                                 ;

    return map_list_of
           (std::string(), root)
           ;
}


CfgParser::CfgParser(std::string configuration)
{
    size_t opening = count(configuration.begin(), configuration.end(), '{');
    size_t closing = count(configuration.begin(), configuration.end(), '}');

    if (opening != closing) {
        throw UserError("The number of opening braces does not much the number of closing braces");
    }

    // break into lines to give later better feedback to users
    boost::replace_all(configuration, ",", ",\n");

    // store the lines in a vector
    std::vector<std::string> lines;
    boost::char_separator<char> sep("\n");
    boost::tokenizer<boost::char_separator<char> > tokens(configuration, sep);
    boost::tokenizer<boost::char_separator<char> >::iterator it;

    // put the configuration into a stream
    std::stringstream ss;

    for (it = tokens.begin(); it != tokens.end(); it++) {
        std::string s = *it;
        boost::trim(s);
        if (!s.empty()) {
            lines.push_back(s);
            ss << s << std::endl;
        }
    }

    try {
        read_json(ss, pt);
    }
    catch (boost::property_tree::json_parser_error &ex) {
        // handle errors in JSON format
        std::string msg = ex.message() + "(around: '" + lines[ex.line() - 1] + "')";
        throw UserError(msg);
    }

    if (validate(pt, shareOnlyCfgTokens)) {
        type = SHARE_ONLY_CFG;
        return;
    }

    if (validate(pt, standaloneSeCfgTokens)) {
        type = STANDALONE_SE_CFG;
        return;
    }

    if (validate(pt, standaloneGrCfgTokens)) {
        type = STANDALONE_GR_CFG;
        return;
    }

    if (validate(pt, sePairCfgTokens)) {
        type = SE_PAIR_CFG;
        return;
    }

    if (validate(pt, grPairCfgTokens)) {
        type = GR_PAIR_CFG;
        return;
    }

    if (validate(pt, activityShareCfgTokens)) {
        type = ACTIVITY_SHARE_CFG;
        return;
    }

    type = NOT_A_CFG;
}


CfgParser::~CfgParser()
{
}


bool CfgParser::validate(boost::property_tree::ptree pt, std::map<std::string, std::set<std::string> > allowed,
    std::string path)
{
    // get the allowed names
    std::set<std::string> names;
    const std::map<std::string, std::set<std::string> >::const_iterator m_it = allowed.find(path);
    if (m_it != allowed.end()) {
        names = m_it->second;
    }

    for (auto it = pt.begin(); it != pt.end(); it++) {
        std::pair<std::string, boost::property_tree::ptree> p = *it;

        // if it's an array entry just continue
        if (p.first.empty()) continue;

        // validate the name
        if (!names.count(p.first)) {
            if (!allTokens.count(p.first)) {
                std::string msg = "unexpected identifier: " + p.first;
                if (!path.empty()) msg += " in " + path + " object";
                throw UserError(msg);
            }
            return false;
        }

        if (p.second.empty()) {
            // check if it should be an object or a value
            if (allowed.find(p.first) != allowed.end()) {
                throw UserError("A member object was expected in " + p.first);
            }
        }
        else {
            // validate the child
            if (!validate(p.second, allowed, p.first)) {
                return false;
            }
        }
    }

    return true;
}


boost::optional<std::string> CfgParser::get_opt(std::string path)
{
    boost::optional<std::string> v;
    try {
        v = pt.get_optional<std::string>(path);
    }
    catch (boost::property_tree::ptree_bad_data &ex) {
        // if the type of the value is wrong throw an exception
        throw UserError("Wrong value type of " + path);
    }
    return v;
}


bool CfgParser::isAuto(std::string path)
{
    std::string v;
    try {
        v = pt.get<std::string>(path);
    }
    catch (boost::property_tree::ptree_bad_path &ex) {
        // if the path is not correct throw en exception
        throw UserError("The " + path + " has to be specified!");
    }
    catch (boost::property_tree::ptree_bad_data &ex) {
        // if the type of the value is wrong throw an exception
        throw UserError("Wrong value type of " + path);
    }

    return v == auto_value;
}

}
}
