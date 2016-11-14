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

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>

#include "common/CfgParser.h"

using fts3::common::CfgParser;
using fts3::common::UserError;


// share only configuration
const std::string SHARE_ONLY_CFG_STR =
    "{"
    "	\"se\":\" srm://se.cern.ch\","
    "	\"active\":true,"
    "	\"in\":[{\"cms\":50},{\"atlas\":50}],"
    "	\"out\":[{\"cms\":60},{\"atlas\":30},{\"public\":10}]"
    "}"
;
// standalone SE configuration
const std::string STANDALONE_SE_CFG_STR =
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
const std::string SE_PAIR_CFG_STR =
    "{"
    "	\"symbolic_name\" : \"se-link\","
    "	\"source_se\" : \" srm://se1.cern.ch\","
    "	\"destination_se\" : \" srm://se2.cern.ch\","
    "	\"share\" : [{\"cms\" : 1}, {\"atlas\" : 2}, {\"public\" : 3}],"
    "	\"protocol\" : [{\"nostreams\" : 12}, {\"urlcopy_tx_to\" : 3600}],"
    "	\"active\":true"
    "}"
;

const std::string STANDALONE_GR_CFG_STR =
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


BOOST_AUTO_TEST_SUITE(common)
BOOST_AUTO_TEST_SUITE(CfgParserTest)

BOOST_AUTO_TEST_CASE (constructor)
{
    // the number of '{' and '}' must be equal
    BOOST_CHECK_THROW(CfgParser p("{{{}}"), UserError);
    // it must be in valid json format
    BOOST_CHECK_THROW(CfgParser p("{lalala}"), UserError);
    // it must not use invalid tokens
    BOOST_CHECK_THROW(CfgParser p("{\"invalid token\" : 8}"), UserError);
    // try parsing share only config
    fts3::common::CfgParser share_only_cfg (SHARE_ONLY_CFG_STR);
    BOOST_CHECK_EQUAL(share_only_cfg.getCfgType(), CfgParser::SHARE_ONLY_CFG);
    // try parsing standalone se config
    CfgParser standalone_se_cfg(STANDALONE_SE_CFG_STR);
    BOOST_CHECK_EQUAL(standalone_se_cfg.getCfgType(), CfgParser::STANDALONE_SE_CFG);
    // try parsing se pair config
    CfgParser se_pair_cfg(SE_PAIR_CFG_STR);
    BOOST_CHECK_EQUAL(se_pair_cfg.getCfgType(), CfgParser::SE_PAIR_CFG);
    // try parsing standalone group configuration
    CfgParser standalone_gr_cfg(STANDALONE_GR_CFG_STR);
    BOOST_CHECK_EQUAL(standalone_gr_cfg.getCfgType(), CfgParser::STANDALONE_GR_CFG);
}


BOOST_AUTO_TEST_CASE (CfgParserGetOpt)
{
    CfgParser parser(STANDALONE_SE_CFG_STR);
    boost::optional<std::string> val = parser.get_opt("se");

    BOOST_CHECK(val.is_initialized());
    BOOST_CHECK_EQUAL(*val, "srm://se.cernc.h");
    BOOST_CHECK(!parser.get_opt("nanana").is_initialized());
}


BOOST_AUTO_TEST_CASE (CfgParserIsAuto)
{
    CfgParser parser(STANDALONE_SE_CFG_STR);
    BOOST_CHECK(!parser.isAuto("in.protocol"));
    BOOST_CHECK(parser.isAuto("out.protocol"));
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
