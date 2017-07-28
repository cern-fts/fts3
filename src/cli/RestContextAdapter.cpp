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

#include "RestContextAdapter.h"

#include "rest/RestSubmission.h"
#include "rest/RestDeletion.h"
#include "rest/RestBanning.h"
#include "rest/RestModifyJob.h"
#include "rest/HttpRequest.h"
#include "rest/ResponseParser.h"

#include "delegation/RestDelegator.h"

#include "exception/rest_invalid.h"
#include "exception/rest_failure.h"
#include "exception/cli_exception.h"

#include <iostream>
#include <sstream>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple_io.hpp>

namespace fts3
{
namespace cli
{

void RestContextAdapter::getInterfaceDetails()
{
    std::stringstream ss;
    HttpRequest http (endpoint, capath, proxy, insecure, ss);

    try {
        http.get();
        ResponseParser parser(ss);

        version += parser.get("api.major");
        version += "." + parser.get("api.minor");
        version += "." + parser.get("api.patch");

        interface = version;
        metadata  = "fts3-rest-" + version;

        schema += parser.get("schema.major");
        schema += "." + parser.get("schema.minor");
        schema += "." + parser.get("schema.patch");
    } catch(rest_failure const &ex) {
        std::string msg = "Error while fetching interface details: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply while fetching "
                          "interface details: " + std::string(ex.what());
        throw cli_exception(msg);
    }
}

void RestContextAdapter::blacklistDn(std::string subject, std::string status, int timeout, bool mode)
{
    std::stringstream ss;
    RestBanning ban(subject, "", status, timeout, mode, true);

    ss << ban.body();

    std::string url = endpoint + ban.resource();
    HttpRequest http (url, capath, proxy, insecure, ss, "affected");

    try {
        ban.do_http_action(http);
        // for the banning case response should include affected job ids
        // no response data for unbanning
    } catch(rest_failure const &ex) {
        std::string msg = "Error while blacklisting the DN: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply while DN "
                          "blacklisting: " + std::string(ex.what());
        throw cli_exception(msg);
    }
}

void RestContextAdapter::blacklistSe(std::string name, std::string vo, std::string status, int timeout, bool mode)
{
    std::stringstream ss;
    RestBanning ban(name, vo, status, timeout, mode, false);

    ss << ban.body();

    std::string url = endpoint + ban.resource();
    HttpRequest http (url, capath, proxy, insecure, ss, "affected");

    try {
        ban.do_http_action(http);
        // for the banning case response should include affected job ids
        // no response data for unbanning
    } catch(rest_failure const &ex) {
        std::string msg = "Error while blacklisting the SE: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply while SE "
                          "blacklisting: " + std::string(ex.what());
        throw cli_exception(msg);
    }
}

void RestContextAdapter::debugSet(std::string source, std::string destination, unsigned level)
{
    std::string url = endpoint + "/config/debug";
    char prefix = '?';

    if (!source.empty()) {
        url += prefix;
        url += "source_se=";
        url += HttpRequest::urlencode(source);
        prefix = '&';
    }
    if (!destination.empty()) {
        url += prefix;
        url += "dest_se=";
        url += HttpRequest::urlencode(destination);
        prefix = '&';
    }
    std::stringstream ss;
    ss << level;
    url += prefix;
    url += "debug_level=" + ss.str();

    ss.clear();
    ss.str(std::string());

    HttpRequest http (url, capath, proxy, insecure, ss);

    try {
        http.post();
        ResponseParser response(ss);
        // response is the input dictionary
    } catch(rest_failure const &ex) {
        std::string msg = "Error while doing debugSet: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply while doing "
                          "debugSet: " + std::string(ex.what());
        throw cli_exception(msg);
    }
}

void RestContextAdapter::prioritySet(std::string jobId, int priority)
{
    std::stringstream ss;
    RestModifyJob modify(jobId, priority);

    ss << modify.body();

    std::string url = endpoint + modify.resource();
    HttpRequest http (url, capath, proxy, insecure, ss);

    try {
        modify.do_http_action(http);
        ResponseParser response(ss);
        // response is the modified job
    } catch(rest_failure const &ex) {
        std::string msg = "Error while modifying the job: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply while modifying "
                          "job: " + std::string(ex.what());
        throw cli_exception(msg);
    }
}

std::vector<JobStatus> RestContextAdapter::listRequests (
    std::vector<std::string> const & statuses,
        std::string const & dn,
        std::string const & vo,
        std::string const & /*source*/, std::string const & /*destination*/)
{
    // prefix will be holding '?' at the first concatenation and then '&'
    char prefix = '?';
    std::string url = endpoint + "/jobs";

    if (!dn.empty())
        {
            url += prefix;
            url += "user_dn=";
            url += HttpRequest::urlencode(dn);
            prefix = '&';
        }

    if (!vo.empty())
        {
            url += prefix;
            url += "vo_name=";
            url += HttpRequest::urlencode(vo);
            prefix = '&';
        }

    if (!statuses.empty())
        {
            std::stringstream ss;
            std::string urlwhoami = endpoint + "/whoami";
            HttpRequest http (urlwhoami, capath, proxy, insecure, ss);

            try {
                http.get();
                ResponseParser parser(ss);
                std::string delid = parser.get("delegation_id");
                url += prefix;
                url += "limit=0&dlg_id=" + HttpRequest::urlencode(delid);
                prefix = '&';
            } catch(rest_failure const &ex) {
                std::string msg = "Error while preparing the job list query; "
                                  "fetching the delegation id: "
                                  + std::string(ex.what());
                throw cli_exception(msg);
            } catch(rest_invalid const &ex) {
                std::string msg = "Error reading the server's reply while "
                                  "preparing the job list query; on fetching "
                                  "the delegation id: " + std::string(ex.what());
                throw cli_exception(msg);
            }

            ss.str(std::string());
            ss.clear();

            url += prefix;
            url += "state_in=";
            std::copy(statuses.begin(),statuses.end()-1,std::ostream_iterator<std::string>(ss,","));
            ss << statuses.back();
            url += HttpRequest::urlencode(ss.str());
            prefix = '&';
        }

    std::stringstream ss;
    HttpRequest http (url, capath, proxy, insecure, ss, "jobs");

    try {
        http.get();
        ResponseParser parser(ss);
        return parser.getJobs("jobs");
    } catch(rest_failure const &ex) {
        std::string msg = "Error listing the matching requests: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply containing the "
                          "list of matching requests: " + std::string(ex.what());
        throw cli_exception(msg);
    }
}

std::vector<JobStatus> RestContextAdapter::listDeletionRequests (
    std::vector<std::string> const &statuses,
        std::string const &dn,
        std::string const &vo,
        std::string const &source,
        std::string const &destination)
{
    // for now call the usual listRequests method. The cli may have setup the statuses with a deletion specific
    // state. Otherwise this list may return jobs for non-deletion requests. (depends on FTS-355).
    // Also see comment in command line option help text, in ListTransferCli.cpp
    return listRequests(statuses, dn, vo, source, destination);
}

std::vector< std::pair<std::string, std::string> > RestContextAdapter::cancel(std::vector<std::string> const & jobIds)
{
    std::vector<std::string>::const_iterator itr;

    std::vector< std::pair< std::string, std::string> > ret;

    for (itr = jobIds.begin(); itr != jobIds.end(); ++itr)
        {
            std::stringstream ss;
            std::string url = endpoint + "/jobs/" + *itr;
            HttpRequest http (url, capath, proxy, insecure, ss);
            bool isLast = (itr+1 == jobIds.end());

            try {
                http.del();
                ResponseParser response(ss);
                ret.push_back(std::make_pair(response.get("job_id"), response.get("job_state")));
            } catch(rest_failure const &ex) {
                if (ex.getCode() == 404) {
                    ret.push_back(std::make_pair(*itr,std::string("DOES_NOT_EXIST")));
                } else {
                    std::string msg = "Error canceling job " + *itr + ": "
                                      + std::string(ex.what()) + ".";
                    if (!isLast) {
                        msg += " Not proceeding with remaining jobs.";
                    }
                    throw cli_exception(msg);
                }
            } catch(rest_invalid const &ex) {
                std::string msg = "Error reading the server's reply when "
                                  "canceling job " + *itr + ": " + ex.what() + ".";
                if (!isLast) {
                    msg += " Not proceeding with remaining jobs.";
                }
                throw cli_exception(msg);
            }
        }

    return ret;
}


boost::tuple<int, int>  RestContextAdapter::cancelAll(const std::string &vo)
{
    std::string url = endpoint;
    if (!vo.empty()) {
        url += "/jobs/vo/" + vo;
    } else {
        url += "/jobs/all";
    }

    std::stringstream ss;
    HttpRequest http (url, capath, proxy, insecure, ss);

    boost::tuple<int, int> result;

    try {
        http.del();
        ResponseParser response(ss);
        result = boost::make_tuple(response.get<int>("affected_jobs"), response.get<int>("affected_files"));
    } catch(rest_failure const &ex) {
        std::string msg = "Error while doing vo wide cancel: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply while doing "
                          "vo wide cancel: " + std::string(ex.what());
        throw cli_exception(msg);
    }
    return result;
}


std::string RestContextAdapter::transferSubmit (std::vector<File> const & files,
    std::map<std::string, std::string> const & parameters, boost::property_tree::ptree const& extraParams)
{
    std::stringstream ss;
    ss << RestSubmission(files, parameters, extraParams);

    std::string url = endpoint + "/jobs";
    HttpRequest http (url, capath, proxy, insecure, ss);

    try {
        http.put();
        ResponseParser response(ss);
        return response.get("job_id");
    } catch(rest_failure const &ex) {
        std::string msg = "Error submitting the job: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply for the jobid of "
                          "the submitted job: " + std::string(ex.what());
        throw cli_exception(msg);
    }
}


std::string RestContextAdapter::deleteFile (const std::vector<std::string>& filesForDelete)
{
    std::stringstream ss;
    ss << RestDeletion(filesForDelete);

    std::string url = endpoint + "/jobs";
    HttpRequest http (url, capath, proxy, insecure, ss);

    try {
        http.put();
        ResponseParser response(ss);
        return response.get("job_id");
    } catch(rest_failure const &ex) {
        std::string msg = "Error submitting the delete job: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply for the jobid of "
                          "the submitted delete job: " + std::string(ex.what());
        throw cli_exception(msg);
    }
}


JobStatus RestContextAdapter::getTransferJobStatus (std::string const & jobId, bool archive)
{
    std::string url = endpoint;

    if (archive) {
        url += "/archive/";
    } else {
        url += "/jobs/";
    }
    url += jobId;

    std::stringstream ss;
    HttpRequest http (url, capath, proxy, insecure, ss);

    try {
        http.get();
        ResponseParser response(ss);

        return JobStatus(
                   response.get("job_id"),
                   response.get("job_state"),
                   response.get("user_dn"),
                   response.get("reason"),
                   response.get("vo_name"),
                   ResponseParser::restGmtToLocal(response.get("submit_time")),
                   -1, // this is never shown so we don't care
                   boost::lexical_cast<int>(response.get("priority"))
               );
    } catch(rest_failure const &ex) {
        std::string msg = "Error getting the job status: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply with the status "
                          "of the job: " + std::string(ex.what());
        throw cli_exception(msg);
    }
}


JobStatus RestContextAdapter::getTransferJobSummary (std::string const & jobId, bool archive)
{
    // first get the files
    std::string url_files = endpoint;

    if (archive) {
        url_files += "/archive/" + jobId;
    } else {
        url_files += "/jobs/" + jobId + "/files";
    }

    // for non-archive jobs this will return an array at top level containing
    // information for each file. archive jobs will return the job info plus
    // a "files" entry containing the array
    std::stringstream ss_files;
    HttpRequest http_files (url_files, capath, proxy, insecure, ss_files, "files");

    std::stringstream ss(ss_files.str());

    try {
        http_files.get();
        ResponseParser response_files(ss_files);

        JobStatus::JobSummary summary (
            response_files.getNb("files", "ACTIVE"),
            response_files.getNb("files", "READY"),
            response_files.getNb("files", "CANCELED"),
            response_files.getNb("files", "FINISHED"),
            response_files.getNb("files", "SUBMITTED"),
            response_files.getNb("files", "FAILED"),
            response_files.getNb("files", "STAGING"),
            response_files.getNb("files", "STARTED"),
            response_files.getNb("files", "DELETE")
        );

        if (!archive) {
            // get the job itself
            ss.clear();
            ss.str(std::string());
            std::string url = endpoint + "/jobs/" + jobId;
            HttpRequest http (url, capath, proxy, insecure, ss);
            http.get();
        }

        ResponseParser response(ss);
        return JobStatus(
                   response.get("job_id"),
                   response.get("job_state"),
                   response.get("user_dn"),
                   response.get("reason"),
                   response.get("vo_name"),
                   ResponseParser::restGmtToLocal(response.get("submit_time")),
                   (int)response_files.getFiles("files").size(),
                   boost::lexical_cast<int>(response.get("priority")),
                   summary
               );
    } catch(rest_failure const &ex) {
        std::string msg = "Error getting the job summary: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error while fetching the job summary: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    }
}

std::vector<FileInfo> RestContextAdapter::getFileStatus (std::string const & jobId,
        bool archive, int offset, int limit, bool retries)
{
    std::vector<FileInfo> results;
    std::string url = endpoint;

    if (archive) {
        url += "/archive/" + jobId;
    } else {
        url += "/jobs/" + jobId + "/files";
    }

    try {
        std::stringstream ss;
        // for non-archive jobs this will return an array at top level containing
        // information for each file. archive jobs will return the job info plus
        // a "files" entry containing the array
        HttpRequest http (url, capath, proxy, insecure, ss, "files");
        http.get();
        ResponseParser response(ss);
        results = response.getFiles("files");

        // If files is empty, fallback to data management (i.e. deletion)
        if (results.empty()) {
            url = endpoint + "/jobs/" + jobId + "/dm";
            ss.str(std::string());
            ss.clear();
            HttpRequest httpDm (url, capath, proxy, insecure, ss, "files");
            httpDm.get();
            ResponseParser responseDm(ss);
            results = responseDm.getFiles("files");
        }

    } catch(rest_failure const &ex) {
        std::string msg = "Error getting the file status: "
                          + std::string(ex.what());
        throw cli_exception(msg);
    } catch(rest_invalid const &ex) {
        std::string msg = "Error reading the server's reply with the status of "
                          "the job's files: " + std::string(ex.what());
        throw cli_exception(msg);
    }

    std::vector<FileInfo>::iterator itr = results.end();
    if (offset>0) {
        if (static_cast<std::vector<FileInfo>::size_type>(offset) < results.size()) {
            itr = results.begin() + offset;
        }
        results.erase(results.begin(), itr);
    }
    if (limit>=0 && results.size() > static_cast<std::vector<FileInfo>::size_type>(limit)) {
        itr = results.begin() +	limit;
        results.erase(itr, results.end());
    }

    if (retries && !archive) {
        for(itr=results.begin(); itr != results.end(); ++itr) {
            uint64_t fileId = itr->getFileId();
            std::stringstream ss;
            ss << fileId;
            url = endpoint + "/jobs/" + jobId + "/files/" + ss.str() + "/retries";
            ss.clear();
            ss.str(std::string());
            try {
                HttpRequest http (url, capath, proxy, insecure, ss, "retries");
                http.get();
                ResponseParser response(ss);
                response.setRetries("retries",*itr);
            } catch(rest_failure const &ex) {
                std::string msg = "Error getting file retry information: "
                                  + std::string(ex.what());
                throw cli_exception(msg);
            } catch(rest_invalid const &ex) {
                std::string msg = "Error reading the server's reply containing the "
                                  "file retry information: " + std::string(ex.what());
                throw cli_exception(msg);
            }
        }
    }

    return results;
}

void RestContextAdapter::delegate(std::string const & delegationId, long expirationTime)
{
    // delegate Proxy Certificate
    RestDelegator delegator(endpoint, delegationId, expirationTime, capath, proxy, insecure);
    delegator.delegate();
}

long RestContextAdapter::isCertValid()
{
    RestDelegator delegator(endpoint, std::string(), 0, capath, proxy, insecure);
    return delegator.isCertValid();
}

} /* namespace cli */
} /* namespace fts3 */
