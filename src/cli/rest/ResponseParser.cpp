/*
 * ResponseParser.cpp
 *
 *  Created on: Feb 21, 2014
 *      Author: simonm
 */

#include "ResponseParser.h"

#include "exception/cli_exception.h"

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "unittest/testsuite.h"
#include <sstream>
#endif

using namespace fts3::cli;

ResponseParser::ResponseParser(std::istream& stream)
{
    try
        {
            // parse
            pt::read_json(stream, response);
        }
    catch(pt::json_parser_error& ex)
        {
            throw cli_exception(ex.message());
        }
}

ResponseParser::ResponseParser(std::string const & json)
{
    std::stringstream stream(json);

    try
        {
            // parse
            pt::read_json(stream, response);
        }
    catch(pt::json_parser_error& ex)
        {
            throw cli_exception(ex.message());
        }
}

ResponseParser::~ResponseParser()
{

}

std::string ResponseParser::get(std::string const & path) const
{
    return response.get<std::string>(path);
}

std::vector<JobStatus> ResponseParser::getJobs(std::string const & path) const
{
    pt::ptree const & jobs = response.get_child(path);

    std::vector<JobStatus> ret;
    pt::ptree::const_iterator it;

    for (it = jobs.begin(); it != jobs.end(); ++it)
        {
            JobStatus j(
                it->second.get<std::string>("job_id"),
                it->second.get<std::string>("job_state"),
                it->second.get<std::string>("user_dn"),
                it->second.get<std::string>("reason"),
                it->second.get<std::string>("vo_name"),
                it->second.get<std::string>("submit_time"),
                -1,
                it->second.get<int>("priority")
            );

            ret.push_back(j);
        }

    return ret;
}

std::vector<FileInfo> ResponseParser::getFiles(std::string const & path) const
{
    pt::ptree const & files = response.get_child(path);

    std::vector<FileInfo> ret;
    pt::ptree::const_iterator it;

    for (it = files.begin(); it != files.end(); ++it)
        {
            ret.push_back(FileInfo(it->second));
        }

    return ret;
}

std::vector<DetailedFileStatus> ResponseParser::getDetailedFiles(std::string const & path) const
{
    pt::ptree const & files = response.get_child(path);

    std::vector<DetailedFileStatus> ret;
    pt::ptree::const_iterator it;

    for (it = files.begin(); it != files.end(); ++it)
        {
            ret.push_back(DetailedFileStatus(it->second));
        }

    return ret;
}

int ResponseParser::getNb(std::string const & path, std::string const & state) const
{
    pt::ptree const & files = response.get_child(path);

    int ret = 0;
    pt::ptree::const_iterator it;

    for (it = files.begin(); it != files.end(); ++it)
        {
            if (it->second.get<std::string>("file_state") == state) ++ret;
        }

    return ret;
}

std::vector<Snapshot> ResponseParser::getSnapshot(bool rest) const
{

    if (rest) return get_snapshot_for_rest();
    return get_snapshot_for_soap();
}

std::vector<Snapshot> ResponseParser::get_snapshot_for_rest() const
{
    std::vector<Snapshot> ret;

    pt::ptree const & snapshots = response.get_child("snapshot");
    pt::ptree::const_iterator it;

    for (it = snapshots.begin(); it != snapshots.end(); ++it)
        {
            Snapshot snapshot;

            static std::string const null = "null";

            std::string value = it->second.get<std::string>("active");
            snapshot.active = value == null ? 0 : boost::lexical_cast<int>(value);

            value = it->second.get<std::string>("avg_queued");
            snapshot.avg_queued = value == null ? 0 : boost::lexical_cast<int>(value);

            value = it->second.get<std::string>("failed");
            snapshot.failed = value == null ? 0 : boost::lexical_cast<int>(value);

            value = it->second.get<std::string>("finished");
            snapshot.finished = value == null ? 0 : boost::lexical_cast<int>(value);

            value = it->second.get<std::string>("max_active");
            snapshot.max_active = value == null ? 0 : boost::lexical_cast<int>(value);

            value = it->second.get<std::string>("submitted");
            snapshot.submitted = value == null ? 0 : boost::lexical_cast<int>(value);

            value = it->second.get<std::string>("success_ratio");
            snapshot.success_ratio = value == null ? 0.0 : boost::lexical_cast<double>(value);

            value = it->second.get<std::string>("avg_throughput.15");
            snapshot._15 = value == null ? 0.0 : boost::lexical_cast<double>(value);

            value = it->second.get<std::string>("avg_throughput.30");
            snapshot._30 = value == null ? 0.0 : boost::lexical_cast<double>(value);

            value = it->second.get<std::string>("avg_throughput.5");
            snapshot._5 = value == null ? 0.0 : boost::lexical_cast<double>(value);

            value = it->second.get<std::string>("avg_throughput.60");
            snapshot._60 = value == null ? 0.0 : boost::lexical_cast<double>(value);


            value = it->second.get<std::string>("frequent_error.count");
            snapshot.frequent_error.first = value == null ? 0 : boost::lexical_cast<int>(value);
            snapshot.frequent_error.second = it->second.get<std::string>("frequent_error.reason");

            snapshot.dst_se = it->second.get<std::string>("dest_se");
            snapshot.src_se = it->second.get<std::string>("source_se");
            snapshot.vo = it->second.get<std::string>("vo_name");

            ret.push_back(snapshot);
        }

    return ret;
}

std::vector<Snapshot> ResponseParser::get_snapshot_for_soap() const
{
    std::vector<Snapshot> ret;

    pt::ptree const & snapshots = response.get_child("snapshot");
    pt::ptree::const_iterator it;
    for (it = snapshots.begin(); it != snapshots.end(); ++it)
        {
            Snapshot snapshot;

            snapshot.vo = it->second.get<std::string>("VO");
            snapshot.src_se = it->second.get<std::string>("Source endpoint");
            snapshot.dst_se = it->second.get<std::string>("Destination endpoint");
            snapshot.active = it->second.get<int>("Current active transfers");
            snapshot.max_active = it->second.get<int>("Max active transfers");
            snapshot.finished = it->second.get<int>("Number of finished (last hour)");
            snapshot.failed = it->second.get<int>("Number of failed (last hour)");
            snapshot.submitted = it->second.get<int>("Number of queued");

            std::string value = it->second.get<std::string>("Avg throughput (last 60min)");
            std::stringstream ss (value);
            ss >> snapshot._60;

            value = it->second.get<std::string>("Avg throughput (last 30min)");
            ss.str(value);
            ss >> snapshot._30;

            value = it->second.get<std::string>("Avg throughput (last 15min)");
            ss.str(value);
            ss >> snapshot._15;

            value = it->second.get<std::string>("Avg throughput (last 5min)");
            ss.str(value);
            ss >> snapshot._5;

            value = it->second.get<std::string>("Link efficiency (last hour)");
            ss.str(value);
            ss >> snapshot.success_ratio;

            value = it->second.get<std::string>("Most frequent error (last hour)");
            ss.str(value);
            ss >> snapshot.frequent_error.first;

            const std::string delimiter("times: ");
            snapshot.frequent_error.second = value.substr(value.find(delimiter) + delimiter.size());

            ret.push_back(snapshot);
        }

    return ret;
}

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(ResponseParserTest)

BOOST_AUTO_TEST_CASE (ResponseParser_get)
{
    std::stringstream resp;

    resp << "{\"job_state\": \"FAILED\"}";

    ResponseParser parser (resp);
    BOOST_CHECK_EQUAL(parser.get("job_state"), "FAILED");
    BOOST_CHECK_THROW(parser.get("job_stateeee"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE (ResponseParser_get_snapshot)
{
    // TODO
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // FTS3_COMPILE_WITH_UNITTESTS
