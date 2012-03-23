/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

/** \file parser.cpp Implementation of Parser class. */

#include "cli_dev.h"
#include "parser.h"

#include "common/logger.h"
#include "common/error.h"

#include <boost/foreach.hpp>
#include<boost/tokenizer.hpp>

#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

using namespace FTS3_COMMON_NAMESPACE;

FTS3_CLI_NAMESPACE_START

/* -------------------------------------------------------------------------- */

Parser::TokenizerResultType Parser::Tokenize (const std::string& line)
{
    typedef boost::tokenizer<boost::char_separator<char> > Tokenizer;
    boost::char_separator<char> sep("=");
    Tokenizer tok(line, sep);

    //if (tok.begin() == NULL)

    std::vector<std::string> tokens (tokens.begin(), tokens.end());



    if (tokens.size() != 2)
    {
        FTS3_COMMON_EXCEPTION_THROW (
            Err_Custom("Syntax error in config data \"" + line + "\"")
        );
    }

    TokenizerResultType result;


    return result;
}

/* -------------------------------------------------------------------------- */

Parser::ConfigDataType Parser::Parse (const Parser::RawDataType& raw)
{
    ConfigDataType result;

    BOOST_FOREACH (std::string line, raw)
    {
        std::pair<std::string, std::string> tokenized = Tokenize(line);
        result[tokenized.first] = tokenized.second;
    }

    return result;
}

/* -------------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST

/** Test if base constructors have been called */
BOOST_FIXTURE_TEST_CASE (Test_Parser_good_case, Parser)
{
    Parser::RawDataType raw;
    raw.push_back("Key=Value");
    Parser::ConfigDataType config = Parser::Parse(raw);
    BOOST_CHECK_EQUAL (config["Key"], std::string("Value"));
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Test_Parser_Tokenize_Good_case, Parser)
{
    TokenizerResultType res = Parser::Tokenize("Key=Value");
    BOOST_CHECK_EQUAL (res.first, std::string("Key"));
    BOOST_CHECK_EQUAL (res.second, std::string("Value"));
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

/* -------------------------------------------------------------------------- */

FTS3_CLI_NAMESPACE_END

