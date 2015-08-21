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

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "unittest/testsuite.h"
#endif

namespace fts3
{
namespace common
{

using namespace boost::assign;

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

// share only configuration
const std::string share_only_cfg_str =
    "{"
    "	\"se\":\" srm://se.cern.ch\","
    "	\"active\":true,"
    "	\"in\":[{\"cms\":50},{\"atlas\":50}],"
    "	\"out\":[{\"cms\":60},{\"atlas\":30},{\"public\":10}]"
    "}"
    ;
// standalone SE configuration
const std::string standalone_se_cfg_str =
    "{"
    "	\"se\" : \"srm://se.cernc.h\","
    "	\"active\" : true,"
    "	\"in\" : {"
    "	   \"share\" : [{\"public\" : 5}],"
    "	   \"protocol\" : [{\"nostreams\" : 10}]"
    "	},"
    "	\"out\" : {"
    "	   \"share\" : [{\"public\" : 4}],"
    "	   \"protocol\" : \"auto\""
    "	}"
    "}"
    ;
// SE pair configuration
const std::string se_pair_cfg_str=
    "{"
    "	\"symbolic_name\" : \"se-link\","
    "	\"source_se\" : \" srm://se1.cern.ch\","
    "	\"destination_se\" : \" srm://se2.cern.ch\","
    "	\"share\" : [{\"cms\" : 1}, {\"atlas\" : 2}, {\"public\" : 3}],"
    "	\"protocol\" : [{\"nostreams\" : 12}, {\"urlcopy_tx_to\" : 3600}],"
    "	\"active\":true"
    "}"
    ;

const std::string standalone_gr_cfg_str =
    "{"
    "\"group\" : \"gr1\","
    "\"members\" : [\" srm://se1.cern.ch\", \" srm://se2.cern.ch\"],"
    "\"active\" : true,"
    "\"in\" : {"
    "\"share\" : [{\"cms\" : 12}, {\"atlas\" : 12}],"
    "\"protocol\" : [{\"nostreams\" : 10}, {\"urlcopy_tx_to\" : 3600}]"
    "},\"out\" : {"
    "\"share\" : [{\"cms\" : 10}, {\"atlas\" : 10}],"
    "\"protocol\" : [{\"nostreams\" : 10}, {\"urlcopy_tx_to\" : 3600}]"
    "}"
    "}"
    ;

#endif


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

    if (opening != closing)
        {
            throw Err_Custom("The number of opening braces does not much the number of closing braces");
        }

    // break into lines to give later better feedback to users
    replace_all(configuration, ",", ",\n");

    // store the lines in a vector
    std::vector<std::string> lines;
    char_separator<char> sep("\n");
    tokenizer< char_separator<char> > tokens(configuration, sep);
    tokenizer< char_separator<char> >::iterator it;

    // put the configuration into a stream
    std::stringstream ss;

    for(it = tokens.begin(); it != tokens.end(); it++)
        {
            std::string s = *it;
            trim(s);
            if (!s.empty())
                {
                    lines.push_back(s);
                    ss << s << std::endl;
                }
        }

    try
        {
            // parse
            read_json(ss, pt);

        }
    catch(json_parser_error& ex)
        {
            // handle errors in JSON format
            std::string msg =
                ex.message() +
                "(around: '" + lines[ex.line() - 1] + "')"
                ;

            throw Err_Custom(msg);
        }

    if (validate(pt, shareOnlyCfgTokens))
        {
            type = SHARE_ONLY_CFG;
            return;
        }

    if (validate(pt, standaloneSeCfgTokens))
        {
            type = STANDALONE_SE_CFG;
            return;
        }

    if (validate(pt, standaloneGrCfgTokens))
        {
            type = STANDALONE_GR_CFG;
            return;
        }

    if (validate(pt, sePairCfgTokens))
        {
            type = SE_PAIR_CFG;
            return;
        }

    if (validate(pt, grPairCfgTokens))
        {
            type = GR_PAIR_CFG;
            return;
        }

    if (validate(pt, activityShareCfgTokens))
        {
            type = ACTIVITY_SHARE_CFG;
            return;
        }

    type = NOT_A_CFG;
}

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
BOOST_AUTO_TEST_SUITE(common)
BOOST_AUTO_TEST_SUITE(CfgParserTest)

BOOST_AUTO_TEST_CASE (constructor)
{
    // the number of '{' and '}' must be equal
    BOOST_CHECK_THROW(CfgParser p("{{{}}"), Err_Custom);
    // it must be in valid json format
    BOOST_CHECK_THROW(CfgParser p("{lalala}"), Err_Custom);
    // it must not use invalid tokens
    BOOST_CHECK_THROW(CfgParser p("{\"invalid token\" : 8}"), Err_Custom);
    // try parsing share only config
    CfgParser share_only_cfg (share_only_cfg_str);
    BOOST_CHECK_EQUAL(share_only_cfg.getCfgType(), CfgParser::SHARE_ONLY_CFG);
    // try parsing standalone se config
    CfgParser standalone_se_cfg(standalone_se_cfg_str);
    BOOST_CHECK_EQUAL(standalone_se_cfg.getCfgType(), CfgParser::STANDALONE_SE_CFG);
    // try parsing se pair config
    CfgParser se_pair_cfg(se_pair_cfg_str);
    BOOST_CHECK_EQUAL(se_pair_cfg.getCfgType(), CfgParser::SE_PAIR_CFG);
    // try parsing standalone group configuration
    CfgParser standalone_gr_cfg(standalone_gr_cfg_str);
    BOOST_CHECK_EQUAL(standalone_gr_cfg.getCfgType(), CfgParser::STANDALONE_GR_CFG);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // FTS3_COMPILE_WITH_UNITTESTS

CfgParser::~CfgParser()
{

}

bool CfgParser::validate(ptree pt, std::map< std::string, std::set <std::string> > allowed, std::string path)
{

    // get the allowed names
    std::set<std::string> names;
    const std::map< std::string, std::set<std::string> >::const_iterator m_it = allowed.find(path);
    if (m_it != allowed.end())
        {
            names = m_it->second;
        }

    ptree::iterator it;
    for (it = pt.begin(); it != pt.end(); it++)
        {
            std::pair<std::string, ptree> p = *it;

            // if it's an array entry just continue
            if (p.first.empty()) continue;

            // validate the name
            if (!names.count(p.first))
                {
                    if (!allTokens.count(p.first))
                        {
                            std::string msg = "unexpected identifier: " + p.first;
                            if (!path.empty()) msg += " in " + path + " object";
                            throw Err_Custom(msg);
                        }
                    return false;
                }

            if (p.second.empty())
                {
                    // check if it should be an object or a value
                    if(allowed.find(p.first) != allowed.end())
                        {
                            throw Err_Custom("A member object was expected in " + p.first);
                        }
                }
            else
                {
                    // validate the child
                    if (!validate(p.second, allowed, p.first))
                        {
                            return false;
                        }
                }
        }

    return true;
}

optional<std::string> CfgParser::get_opt(std::string path)
{

    optional<std::string> v;
    try
        {

            v = pt.get_optional<std::string>(path);

        }
    catch (ptree_bad_data& ex)
        {
            // if the type of the value is wrong throw an exception
            throw Err_Custom("Wrong value type of " + path);
        }

    return v;
}

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
BOOST_AUTO_TEST_SUITE( common )
BOOST_AUTO_TEST_SUITE(CfgParserTest)

BOOST_AUTO_TEST_CASE (CfgParser_get_opt)
{
    CfgParser parser(standalone_se_cfg_str);
    optional<std::string> val = parser.get_opt("se");

    BOOST_CHECK(val.is_initialized());
    BOOST_CHECK_EQUAL(*val, "srm://se.cernc.h");
    BOOST_CHECK(!parser.get_opt("nanana").is_initialized());
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // FTS3_COMPILE_WITH_UNITTESTS

bool CfgParser::isAuto(std::string path)
{

    std::string v;
    try
        {

            v = pt.get<std::string>(path);

        }
    catch (ptree_bad_path& ex)
        {
            // if the path is not correct throw en exception
            throw Err_Custom("The " + path + " has to be specified!");
        }
    catch (ptree_bad_data& ex)
        {
            // if the type of the value is wrong throw an exception
            throw Err_Custom("Wrong value type of " + path);
        }

    return v == auto_value;
}


#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
BOOST_AUTO_TEST_SUITE( common )
BOOST_AUTO_TEST_SUITE(CfgParserTest)

BOOST_AUTO_TEST_CASE (CfgParser_isAuto)
{
    CfgParser parser(standalone_se_cfg_str);
    BOOST_CHECK(!parser.isAuto("in.protocol"));
    BOOST_CHECK(parser.isAuto("out.protocol"));
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // FTS3_COMPILE_WITH_UNITTESTS
}
}

