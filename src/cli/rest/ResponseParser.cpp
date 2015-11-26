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

#include "ResponseParser.h"

#include "exception/rest_invalid.h"

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>


using namespace fts3::cli;

ResponseParser::ResponseParser(std::istream& stream)
{
    parse(stream);
}

ResponseParser::ResponseParser(std::string const & json)
{
    parse(json);
}

ResponseParser::~ResponseParser()
{

}

void ResponseParser::parse(std::istream& stream)
{
    try
        {
            // parse
            pt::read_json(stream, response);
        }
    catch(pt::json_parser_error& ex)
        {
            throw rest_invalid(ex.message());
        }
}

void ResponseParser::parse(std::string const &json)
{
    std::stringstream stream(json);
    parse(stream);
}

// Times returned by the rest interface are in UTC. By convention
// these are displayed to the user in local time. This method
// converts from the format returned by the rest interface to a
// slightly different format, in local time.
//
// e.g. 2015-11-12T04:45:28 -> 2015-11-12 05:45:28
//
// static member function
//
std::string ResponseParser::restGmtToLocal(std::string gmt)
{
    tm tmtime;
    memset(&tmtime, 0, sizeof(tmtime));
    strptime(gmt.c_str(), "%Y-%m-%dT%H:%M:%S", &tmtime);
    time_t t = timegm(&tmtime);
    char time_buff[20];
    strftime(time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", localtime_r(&t,&tmtime));
    return std::string(time_buff);
}

std::string ResponseParser::get(std::string const & path) const
{
    if (path == "submit_time") {
        return ResponseParser::restGmtToLocal(response.get<std::string>(path));
    }
    return response.get<std::string>(path);
}

void ResponseParser::setRetries(std::string const &path, FileInfo &fi)
{
    pt::ptree const &r = response.get_child(path);
    fi.setRetries(r);
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
                ResponseParser::restGmtToLocal(it->second.get<std::string>("submit_time")),
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

