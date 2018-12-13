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
#include <boost/property_tree/json_parser.hpp>

#include "cli/rest/RestSubmission.h"
#include "cli/JobParameterHandler.h"

using fts3::cli::File;
using fts3::cli::RestSubmission;
using fts3::cli::JobParameterHandler;
namespace pt = boost::property_tree;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(RestSubmissionTest)


static const std::string submit1 =
    "{"
    "  \"files\": ["
    "    {"
    "      \"sources\": [\"root://source/file\"],"
    "      \"destinations\": [\"root://dest/file\"]"
    "    }"
    "  ]"
    "}"
;


static const std::string submit2 =
    "{"
    "  \"files\": ["
    "    {"
    "      \"sources\": [\"root://source/file\"],"
    "      \"destinations\": [\"root://dest/file\"],"
    "      \"metadata\": \"User defined metadata\","
    "      \"filesize\": 1024,"
    "      \"checksum\": \"adler32:1234\""
    "    }"
    "  ]"
    "}"

;


static const std::string submit3 =
    "{"
    "  \"files\": ["
    "    {"
    "      \"sources\": [\"root://source/file\"],"
    "      \"destinations\": [\"root://dest/file\"],"
    "      \"metadata\": \"User defined metadata\","
    "      \"filesize\": 1024,"
    "      \"checksum\": \"adler32:1234\""
    "    }"
    "  ],"
    "  \"params\": {"
    "    \"verify_checksum\": true,"
    "    \"retry\": 2"
    "  }"
    "}"
;


static const std::string submit4 =
    "{"
    "  \"files\": ["
    "    {"
    "      \"sources\": [\"root://source/file\", \"root://source2/file2\"],"
    "      \"destinations\": [\"root://dest/file\", \"root://dest2/file2\"]"
    "    }"
    "  ]"
    "}"
;


static const std::string submit5 =
    "{"
    "  \"files\": ["
    "    {"
    "      \"sources\": [\"root://source/file\", \"root://source2/file2\"],"
    "      \"destinations\": [\"root://dest/file\", \"root://dest2/file2\"],"
    "      \"metadata\": \"with filesize\","
    "      \"filesize\": 1024,"
    "      \"checksum\": \"adler32:1234\""
    "    }"
    "  ],"
    "  \"params\": {"
    "    \"verify_checksum\": true,"
    "    \"reuse\": true,"
    "    \"spacetoken\": \"blablabla\","
    "    \"bring_online\": 24,"
    "    \"copy_pin_lifetime\": 108,"
    "    \"job_metadata\": \"retry two times\","
    "    \"source_spacetoken\": \"nanana\","
    "    \"overwrite\": true,"
    "    \"gridftp\": \"eeeeeeee\","
    "    \"multihop\": true,"
    "    \"retry\": 2"
    "  }"
    "}"
;


static const std::string submit5_quoted =
    "{"
    "  \"files\": ["
    "    {"
    "      \"sources\": [\"root://source/file\", \"root://source2/file2\"],"
    "      \"destinations\": [\"root://dest/file\", \"root://dest2/file2\"],"
    "      \"metadata\": \"with filesize\","
    "      \"filesize\": \"1024\","
    "      \"checksum\": \"adler32:1234\""
    "    }"
    "  ],"
    "  \"params\": {"
    "    \"verify_checksum\": true,"
    "    \"reuse\": \"true\","
    "    \"spacetoken\": \"blablabla\","
    "    \"bring_online\": \"24\","
    "    \"copy_pin_lifetime\": \"108\","
    "    \"job_metadata\": \"retry two times\","
    "    \"source_spacetoken\": \"nanana\","
    "    \"overwrite\": \"true\","
    "    \"gridftp\": \"eeeeeeee\","
    "    \"multihop\": \"true\","
    "    \"retry\": \"2\""
    "  }"
    "}"
;


struct RestSubmissionFixture : public RestSubmission
{

    static std::string get_expected(std::string const &json_str)
    {
        std::stringstream stream;
        // put the JSON message into stream
        stream << json_str;
        pt::ptree json;
        // create a ptree from the stream
        pt::read_json(stream, json);
        // clear the stream
        stream.str("");
        // put the expected result again into stream
        // (this way the white-spaces should be the same as in the result)
        RestSubmission::to_output(stream, json);
        return stream.str();
    }

    static std::vector <std::string> get_urls(pt::ptree const &p)
    {
        std::vector <std::string> ret;

        for (auto it = p.begin(); it != p.end(); ++it) {
            ret.push_back(it->second.get_value<std::string>());
        }

        return ret;
    }

    static std::vector <File> get_files(std::string const &json_str)
    {
        std::stringstream stream;
        // put the JSON message into stream
        stream << json_str;
        pt::ptree json;
        // create a ptree from the stream
        pt::read_json(stream, json);

        std::vector <File> ret;

        pt::ptree const &files = json.get_child("files");

        for (auto it = files.begin(); it != files.end(); ++it) {
            File file;
            file.sources = get_urls(it->second.get_child("sources"));
            file.destinations = get_urls(it->second.get_child("destinations"));
            file.metadata = it->second.get_optional<std::string>("metadata");
            file.file_size = it->second.get_optional<double>("filesize");
            boost::optional <std::string> checksum = it->second.get_optional<std::string>("checksum");
            if (checksum) file.checksum = checksum;

            ret.push_back(file);
        }

        return ret;
    }

    static std::map <std::string, std::string> get_params(std::string const &json_str)
    {
        std::stringstream stream;
        // put the JSON message into stream
        stream << json_str;
        pt::ptree json;
        // create a ptree from the stream
        pt::read_json(stream, json);

        std::map <std::string, std::string> ret;

        boost::optional <std::string> verify_checksum = json.get_optional<std::string>("params.verify_checksum");
        if (verify_checksum)
            ret[JobParameterHandler::CHECKSUM_METHOD] = "both";

        boost::optional <std::string> reuse = json.get_optional<std::string>("params.reuse");
        if (reuse)
            ret[JobParameterHandler::REUSE] = "Y";

        boost::optional <std::string> spacetoken = json.get_optional<std::string>("params.spacetoken");
        if (spacetoken)
            ret[JobParameterHandler::SPACETOKEN] = *spacetoken;

        boost::optional <std::string> bring_online = json.get_optional<std::string>("params.bring_online");
        if (bring_online)
            ret[JobParameterHandler::BRING_ONLINE] = *bring_online;

        boost::optional <std::string> copy_pin_lifetime = json.get_optional<std::string>("params.copy_pin_lifetime");
        if (copy_pin_lifetime)
            ret[JobParameterHandler::COPY_PIN_LIFETIME] = *copy_pin_lifetime;

        boost::optional <std::string> job_metadata = json.get_optional<std::string>("params.job_metadata");
        if (job_metadata)
            ret[JobParameterHandler::JOB_METADATA] = *job_metadata;

        boost::optional <std::string> source_spacetoken = json.get_optional<std::string>("params.source_spacetoken");
        if (source_spacetoken)
            ret[JobParameterHandler::SPACETOKEN_SOURCE] = *source_spacetoken;

        boost::optional <std::string> overwrite = json.get_optional<std::string>("params.overwrite");
        if (overwrite)
            ret[JobParameterHandler::OVERWRITEFLAG] = "Y";

        boost::optional <std::string> gridftp = json.get_optional<std::string>("params.gridftp");
        if (gridftp)
            ret[JobParameterHandler::GRIDFTP] = *gridftp;

        boost::optional <std::string> multihop = json.get_optional<std::string>("params.multihop");
        if (multihop)
            ret[JobParameterHandler::MULTIHOP] = "Y";

        boost::optional <std::string> retry = json.get_optional<std::string>("params.retry");
        if (retry)
            ret[JobParameterHandler::RETRY] = *retry;

        return ret;
    }

    static std::string get_result(std::string const &json_str)
    {
        std::vector <File> files = get_files(json_str);
        std::map <std::string, std::string> parameters = get_params(json_str);

        std::stringstream stream;
        stream << RestSubmission(files, parameters, boost::property_tree::ptree());
        return stream.str();
    }

    static std::string to_array(std::vector <std::string> const &vec)
    {
        // create the array in ptree
        pt::ptree root, array;
        RestSubmission::to_array(vec, array);
        root.push_back(std::make_pair("array", array));
        // convert it to string
        std::stringstream ss;
        RestSubmission::to_output(ss, root);
        // return result
        return ss.str();
    }

    RestSubmission::strip_values;
};


BOOST_AUTO_TEST_CASE (RestSubmissionToArray)
{
    std::string array = "{\"array\" : [\"a\", \"a\", \"a\", \"a\", \"a\"]}";
    std::string expected = RestSubmissionFixture::get_expected(array);
    std::vector<std::string> vec (5, "a");
    std::string result = RestSubmissionFixture::to_array(vec);
    BOOST_CHECK_EQUAL(expected, result);
}


BOOST_AUTO_TEST_CASE(RestSubmissionStripValues)
{
    // 1st
    std::string result = "\"value1\"    :   \"1234\", some txt \"value2\" :       \"null\"";
    std::string expected = "\"value1\"    :   1234, some txt \"value2\" :       null";
    RestSubmissionFixture::strip_values(result, "value1");
    RestSubmissionFixture::strip_values(result, "value2");
    BOOST_CHECK_EQUAL(expected, result);
    // 2nd
    expected = submit5;
    result = RestSubmissionFixture::strip_values(submit5_quoted);
    BOOST_CHECK_EQUAL(expected, result);
}


BOOST_AUTO_TEST_CASE(RestSubmissionStreamOp)
{
    // 1st
    std::string expected = RestSubmissionFixture::get_expected(submit1);
    std::string result = RestSubmissionFixture::get_result(submit1);
    BOOST_CHECK_EQUAL(expected, result);
    // 2nd
    expected = RestSubmissionFixture::get_expected(submit2);
    result = RestSubmissionFixture::get_result(submit2);
    BOOST_CHECK_EQUAL(expected, result);
    // 3rd
    expected = RestSubmissionFixture::get_expected(submit3);
    result = RestSubmissionFixture::get_result(submit3);
    BOOST_CHECK_EQUAL(expected, result);
    // 4th
    expected = RestSubmissionFixture::get_expected(submit4);
    result = RestSubmissionFixture::get_result(submit4);
    BOOST_CHECK_EQUAL(expected, result);
    // 5th
    expected = RestSubmissionFixture::get_expected(submit5);
    result = RestSubmissionFixture::get_result(submit5);
    BOOST_CHECK_EQUAL(expected, result);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
