/*
 * RestSubmission.cpp
 *
 *  Created on: 19 Aug 2014
 *      Author: simonm
 */

#include "RestSubmission.h"

#include "common/JobParameterHandler.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "unittest/testsuite.h"
#include <sstream>
#endif

namespace fts3 {
namespace cli {

using namespace fts3::common;


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
            {"filesize", "verify_checksum", "reuse", "bring_online", "copy_pin_lifetime", "overwrite", "multihop", "retry"};
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
        if (it->metadata)
            element.put("metadata", *it->metadata);
        // add file size if provided
        if (it->file_size)
            element.put("filesize", boost::lexical_cast<std::string>(*it->file_size));
        // add checksum if provided
        if (!it->checksums.empty())
            element.put("checksum", *it->checksums.begin());
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
    param_itr = me.parameters.find(JobParameterHandler::JOB_METADATA);
    if (param_itr != me.parameters.end())
        {
            params.put("job_metadata", param_itr->second);
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

    // add params to root
    if (!params.empty())
        root.push_back(std::make_pair("params", params));

    RestSubmission::to_output(os, root);

    return os;
}

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(RestSubmissionTest)

static std::string submit1 =
"{"
"  \"files\": ["
"    {"
"      \"sources\": [\"root://source/file\"],"
"      \"destinations\": [\"root://dest/file\"]"
"    }"
"  ]"
"}"
;

static std::string submit2 =
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

static std::string submit3 =
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

static std::string submit4 =
"{"
"  \"files\": ["
"    {"
"      \"sources\": [\"root://source/file\", \"root://source2/file2\"],"
"      \"destinations\": [\"root://dest/file\", \"root://dest2/file2\"]"
"    }"
"  ]"
"}"
;

static std::string submit5 =
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

static std::string submit5_quoted =
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
"    \"verify_checksum\": \"true\","
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

    static std::string get_expected(std::string const & json_str)
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

    static std::vector<std::string> get_urls(pt::ptree const & p)
    {
        std::vector<std::string> ret;

        pt::ptree::const_iterator it;
            for (it = p.begin(); it != p.end(); ++it)
            {
                ret.push_back(it->second.get_value<std::string>());
            }

        return ret;
    }

    static std::vector<File> get_files(std::string const & json_str)
    {
        std::stringstream stream;
        // put the JSON message into stream
        stream << json_str;
        pt::ptree json;
        // create a ptree from the stream
        pt::read_json(stream, json);

        std::vector<File> ret;

        pt::ptree const & files = json.get_child("files");

        pt::ptree::const_iterator it;
        for (it = files.begin(); it != files.end(); ++it)
        {
            File file;
            file.sources = get_urls(it->second.get_child("sources"));
            file.destinations = get_urls(it->second.get_child("destinations"));
            file.metadata = it->second.get_optional<std::string>("metadata");
            file.file_size = it->second.get_optional<double>("filesize");
            boost::optional<std::string> checksum = it->second.get_optional<std::string>("checksum");
            if (checksum) file.checksums.push_back(*checksum);

            ret.push_back(file);
        }

        return ret;
    }

    static std::map<std::string, std::string> get_params(std::string const & json_str)
    {
        std::stringstream stream;
        // put the JSON message into stream
        stream << json_str;
        pt::ptree json;
        // create a ptree from the stream
        pt::read_json(stream, json);

        std::map<std::string, std::string> ret;

        boost::optional<std::string> verify_checksum = json.get_optional<std::string>("params.verify_checksum");
        if (verify_checksum)
            ret[JobParameterHandler::CHECKSUM_METHOD] = "compare";

        boost::optional<std::string> reuse = json.get_optional<std::string>("params.reuse");
        if (reuse)
            ret[JobParameterHandler::REUSE] = "Y";

        boost::optional<std::string> spacetoken = json.get_optional<std::string>("params.spacetoken");
        if (spacetoken)
            ret[JobParameterHandler::SPACETOKEN] = *spacetoken;

        boost::optional<std::string> bring_online = json.get_optional<std::string>("params.bring_online");
        if (bring_online)
            ret[JobParameterHandler::BRING_ONLINE] = *bring_online;

        boost::optional<std::string> copy_pin_lifetime = json.get_optional<std::string>("params.copy_pin_lifetime");
        if (copy_pin_lifetime)
            ret[JobParameterHandler::COPY_PIN_LIFETIME] = *copy_pin_lifetime;

        boost::optional<std::string> job_metadata = json.get_optional<std::string>("params.job_metadata");
        if (job_metadata)
            ret[JobParameterHandler::JOB_METADATA] = *job_metadata;

        boost::optional<std::string> source_spacetoken = json.get_optional<std::string>("params.source_spacetoken");
        if (source_spacetoken)
            ret[JobParameterHandler::SPACETOKEN_SOURCE] = *source_spacetoken;

        boost::optional<std::string> overwrite = json.get_optional<std::string>("params.overwrite");
        if (overwrite)
            ret[JobParameterHandler::OVERWRITEFLAG] = "Y";

        boost::optional<std::string> gridftp = json.get_optional<std::string>("params.gridftp");
        if (gridftp)
            ret[JobParameterHandler::GRIDFTP] = *gridftp;

        boost::optional<std::string> multihop = json.get_optional<std::string>("params.multihop");
        if (multihop)
            ret[JobParameterHandler::MULTIHOP] = "Y";

        boost::optional<std::string> retry = json.get_optional<std::string>("params.retry");
        if (retry)
            ret[JobParameterHandler::RETRY] = *retry;

        return ret;
    }

    static std::string get_result(std::string const & json_str)
    {
        std::vector<File> files = get_files(json_str);
        std::map<std::string, std::string> parameters = get_params(json_str);

        std::stringstream stream;
        stream << RestSubmission(files, parameters);
        return stream.str();
    }

    static std::string to_array(std::vector<std::string> const & vec)
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

BOOST_AUTO_TEST_CASE (RestSubmission_to_array)
{
    std::string array = "{\"array\" : [\"a\", \"a\", \"a\", \"a\", \"a\"]}";
    std::string expected = RestSubmissionFixture::get_expected(array);
    std::vector<std::string> vec (5, "a");
    std::string result = RestSubmissionFixture::to_array(vec);
    BOOST_CHECK_EQUAL(expected, result);
}

BOOST_AUTO_TEST_CASE(RestSubmission_strip_values)
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

BOOST_AUTO_TEST_CASE (RestSubmission_stream_op)
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
#endif // FTS3_COMPILE_WITH_UNITTESTS

} /* namespace cli */
} /* namespace fts3 */
