/*
 * Copyright (c) CERN 2015
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

#include <boost/property_tree/json_parser.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>
#include <set>

#include "cli/rest/RestDeletion.h"

using fts3::cli::RestDeletion;
namespace pt = boost::property_tree;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(RestDeletionTest)


BOOST_AUTO_TEST_CASE(DeleteListOfFiles)
{
    std::vector<std::string> files {
        "gsiftp://site01.com/file",
        "gsiftp://site01.com/file2",
        "gsiftp://site01.com/file3"
    };

    RestDeletion del(files);
    std::stringstream output;
    output << del;

    pt::ptree tree;
    pt::json_parser::read_json(output, tree);

    auto fileList = tree.get_child("delete");
    BOOST_CHECK_EQUAL(3, fileList.size());

    std::set<std::string> outFiles;
    for (auto i = fileList.begin(); i != fileList.end(); ++i) {
        outFiles.insert(i->second.get_value<std::string>());
    }

    BOOST_CHECK(outFiles.find("gsiftp://site01.com/file") != outFiles.end());
    BOOST_CHECK(outFiles.find("gsiftp://site01.com/file2") != outFiles.end());
    BOOST_CHECK(outFiles.find("gsiftp://site01.com/file3") != outFiles.end());
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
