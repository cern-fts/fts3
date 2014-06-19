/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * MsgPrinter.cpp
 *
 *  Created on: Dec 11, 2012
 *      Author: simonm
 */

#include "MsgPrinter.h"

#include "common/JobStatusHandler.h"

#include "JsonOutput.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>

#include <utility>
#include <sstream>
#include <algorithm>

namespace fts3
{
namespace cli
{

using namespace boost;
using namespace boost::assign;
using namespace fts3::common;

bool MsgPrinter::verbose = false;

void MsgPrinter::delegation_request_duration(long int h, long int m)
{

    if (!verbose) return;

    if (!json)
        {
            cout << "Requesting delegated proxy for " << h << " hours and " << m << " minutes." << endl;
            return;
        }

    JsonOutput::print("delegation.request.duration", lexical_cast<string>(h) + ":" + lexical_cast<string>(m));
}

void MsgPrinter::delegation_request_retry()
{

    if (!verbose) return;

    if (!json)
        {
            cout << "Retrying!" << endl;
            return;
        }

    JsonOutput::print("delegation.request.retry", "true");
}

void MsgPrinter::delegation_request_error(string error)
{

//	if (!verbose) return;

    if (!json)
        {
            cout << "delegation: " << error << endl;
            return;
        }

    JsonOutput::print("delegation.request.error", error);
}

void MsgPrinter::delegation_request_success(bool b)
{

    if (!verbose) return;

    if (!json)
        {
            if (b) cout << "Credential has been successfully delegated to the service." << endl;
            return;
        }

    stringstream ss;
    ss << std::boolalpha << b;

    JsonOutput::print("delegation.request.delegated_successfully", ss.str());
}

void MsgPrinter::delegation_local_expiration(long int h, long int m)
{

    if (!verbose) return;

    if (!json)
        {
            cout << "Remaining time for the local proxy is " << h << "hours and " << m << " minutes." << endl;
            return;
        }

    JsonOutput::print("delegation.expiration_time.local", lexical_cast<string>(h) + ":" + lexical_cast<string>(m));
}

void MsgPrinter::delegation_service_proxy(long int h, long int m)
{

    if (!verbose) return;

    if (!json)
        {
            cout << "Remaining time for the proxy on the server side is " << h << " hours and " << m << " minutes." << endl;
            return;
        }

    JsonOutput::print("delegation.expiration_time.service", lexical_cast<string>(h) + ":" + lexical_cast<string>(m));
}

void MsgPrinter::delegation_msg(string msg)
{

    if (!verbose) return;

    if (!json)
        {
            cout << msg << endl;
            return;
        }

    JsonOutput::print("delegation.message", msg);
}

void MsgPrinter::endpoint(string endpoint)
{

    if (!json)
        {
            cout << "# Using endpoint: " << endpoint << endl;
            return;
        }

    JsonOutput::print("endpoint", endpoint);
}

void MsgPrinter::service_version(string version)
{

    if (!json)
        {
            cout << "# Service version: " << version << endl;
            return;
        }

    JsonOutput::print("service_version", version);
}

void MsgPrinter::service_interface(string interface)
{

    if (!json)
        {
            cout << "# Interface version: " << interface << endl;
            return;
        }

    JsonOutput::print("service_interface", interface);
}

void MsgPrinter::service_schema(string schema)
{

    if (!json)
        {
            cout << "# Schema version: " << schema << endl;
            return;
        }

    JsonOutput::print("service_schema", schema);
}

void MsgPrinter::service_metadata(string metadata)
{

    if (!json)
        {
            cout << "# Service features: " << metadata << endl;
            return;
        }

    JsonOutput::print("service_metadata", metadata);
}

void MsgPrinter::client_version(string version)
{

    if (!json)
        {
            cout << "# Client version: " << version << endl;
            return;
        }

    JsonOutput::print("client_version", version);
}

void MsgPrinter::client_interface(string interface)
{

    if (!json)
        {
            cout << "# Client interface version: " << interface << endl;
            return;
        }

    JsonOutput::print("client_interface", interface);
}

void MsgPrinter::print_cout(std::pair<std::string, std::string> const & id_status)
{
    std::cout << "job " << id_status.first << ": " << id_status.second << endl;
}

void MsgPrinter::print_json(std::pair<std::string, std::string> const & id_status)
{
    std::map<std::string, std::string> m = boost::assign::map_list_of ("job_id", id_status.first) ("job_state", id_status.second);
    JsonOutput::printArray("job", m);
}

void MsgPrinter::missing_parameter(string name)
{

    if (!json)
        {
            cout << "missing parameter: " << name << endl;
            return;
        }

    JsonOutput::print("error.missing_parameter", name);
}

void MsgPrinter::bulk_submission_error(int line, string msg)
{

    if (!json)
        {
            cout << "submit: in line " << line << ": " << msg << endl;
            return;
        }

    JsonOutput::print("error.line", lexical_cast<string>(line));
    JsonOutput::print("error.message", msg);
}

void MsgPrinter::wrong_endpoint_format(string endpoint)
{

    if (!json)
        {
            cout << "wrongly formated endpoint: " << endpoint << endl;
            return;
        }

    JsonOutput::print("wrong_format.endpoint", endpoint);
}

void MsgPrinter::version(string version)
{

    if (!json)
        {
            cout << "version: " << version << endl;
            return;
        }

    JsonOutput::print("client_version", version);
}

void MsgPrinter::job_id(string job_id)
{

    if (!json)
        {
            cout << job_id << endl;
            return;
        }

    JsonOutput::print("job.job_id", job_id);
}

void MsgPrinter::status(JobStatus js)
{

    if (!json)
        {
            cout << js.jobStatus << endl;
            return;
        }

    map<string, string> object = map_list_of ("job_id", js.jobId) ("status", js.jobStatus);
    JsonOutput::printArray("job", object);
}

void MsgPrinter::error_msg(string msg)
{

    if (!json)
        {
            cout << msg << endl;
            return;
        }

    JsonOutput::print("error.message", msg);
}

void MsgPrinter::gsoap_error_msg(string msg)
{
    // remove backspaces if any in the string
    string::size_type  pos;
    while((pos = msg.find(8)) != string::npos)
        {
            msg.erase(pos, 1);
        }


    pos = msg.find("\"\nDetail: ", 0);

//	regex re (".*\"(.+)\"\nDetail: (.+)\n");

    if (pos == string::npos)
        {
            error_msg(msg);
            return;
        }

    if (!json)
        {
            cout << "error: " << msg << endl;
            return;
        }

    string detail = msg.substr(pos + 10);
    string::size_type size = detail.size();
    if (detail[size - 1] == '\n') detail = detail.substr(0, size - 1);

    pos = msg.find("\"");
    string err_msg;
    if (pos != string::npos) err_msg = msg.substr(pos + 1);
    pos = msg.find("\"");
    if (pos != string::npos) err_msg = msg.substr(0, pos);
    size = err_msg.size();
    if (err_msg[size - 1] == '\n') err_msg = err_msg.substr(0, size - 1);

    JsonOutput::print("error.message", err_msg);
    JsonOutput::print("error.detail", detail);
}

void MsgPrinter::print_cout(JobStatus const & j)
{
    cout << "Request ID: " << j.jobId << endl;
    cout << "Status: " << j.jobStatus << endl;

    // if not verbose return
    if (!verbose) return;

    cout << "Client DN: " << j.clientDn << endl;
    cout << "Reason: " << (j.reason.empty() ? "<None>": j.reason) << endl;
    cout << "Submission time: " << j.submitTime << endl;
    cout << "Files: " << (j.numFiles == -1 ? "n/a" : boost::lexical_cast<std::string>(j.numFiles)) << endl;
    cout << "Priority: " << j.priority << endl;
    cout << "VOName: " << j.voName << endl;
    cout << endl;
}

void MsgPrinter::print_json(JobStatus const & j)
{
    map<string, string> object;

    if (verbose)
        {
            map<string, string> aux = map_list_of
                                      ("job_id", j.jobId)
                                      ("status", j.jobStatus)
                                      ("dn", j.clientDn)
                                      ("reason", j.reason.empty() ? "<None>": j.reason)
                                      ("submision_time", j.submitTime)
                                      ("file_count", (j.numFiles == -1 ? "n/a" : boost::lexical_cast<std::string>(j.numFiles)))
                                      ("priority", lexical_cast<string>(j.priority))
                                      ("vo", j.voName)
                                      ;
            object = aux;
        }
    else
        {
            map<string, string> aux = map_list_of ("job_id", j.jobId) ("status", j.jobStatus);
            object = aux;
        }

    JsonOutput::printArray("job", object);
}

void MsgPrinter::print_cout(tns3__DetailedFileStatus * const & status)
{
	print_json(status);
}

void MsgPrinter::print_json(tns3__DetailedFileStatus * const & status)
{
	map<string, string> object = map_list_of
			("job_id", status->jobId)
			("file_id", lexical_cast<string>(status->fileId))
			("file_state", status->fileState)
			("source_surl", status->sourceSurl)
			("dest_surl", status->destSurl)
			;

	JsonOutput::printArray("job.files", object);
}

void MsgPrinter::job_summary(JobSummary js)
{

    if (!json)
        {
            print_cout(js.status);
            cout << "\tActive: " << js.numActive << endl;
            cout << "\tReady: " << js.numReady << endl;
            cout << "\tCanceled: " << js.numCanceled << endl;
            cout << "\tFinished: " << js.numFinished << endl;
            cout << "\tSubmitted: " << js.numSubmitted << endl;
            cout << "\tFailed: " << js.numFailed << endl;
            return;
        }

    map<string, string> object = map_list_of
                                 ("job_id", js.status.jobId)
                                 ("status", js.status.jobStatus)
                                 ("dn", js.status.clientDn)
                                 ("reason", js.status.reason.empty() ? "<None>": js.status.reason)
                                 ("submision_time", js.status.submitTime)
                                 ("file_count", lexical_cast<string>(js.status.numFiles))
                                 ("priority", lexical_cast<string>(js.status.priority))
                                 ("vo", js.status.voName)

                                 ("summary.active", lexical_cast<string>(js.numActive))
                                 ("summary.ready", lexical_cast<string>(js.numReady))
                                 ("summary.canceled", lexical_cast<string>(js.numCanceled))
                                 ("summary.finished", lexical_cast<string>(js.numFinished))
                                 ("summary.submitted", lexical_cast<string>(js.numSubmitted))
                                 ("summary.failed", lexical_cast<string>(js.numFailed))
                                 ;

    JsonOutput::printArray("job", object);
}

void MsgPrinter::file_list(vector<string> values, vector<string> retries)
{

    enum
    {
        SOURCE,
        DESTINATION,
        STATE,
        RETRIES,
        REASON,
        DURATION
    };

    if (!json)
        {
            cout << "  Source:      " << values[SOURCE] << endl;
            cout << "  Destination: " << values[DESTINATION] << endl;
            cout << "  State:       " << values[STATE] << endl;;
            cout << "  Reason:      " << values[REASON] << endl;
            cout << "  Duration:    " << values[DURATION] << endl;

            if (retries.size() > 0)
                {
                    cout << "  Retries: " << endl;
                    for_each(retries.begin(), retries.end(), cout << ("    " + lambda::_1) << '\n');
                }
            else
                {
                    cout << "  Retries:     " << values[RETRIES] << endl;
                }
            return;
        }

    ptree file;
    file.put("source", values[SOURCE]);
    file.put("destination", values[DESTINATION]);
    file.put("state", values[STATE]);
    file.put("reason", values[REASON]);
    file.put("duration", values[DURATION]);

    if (retries.size() > 0)
        {
            ptree retriesArray;
            vector<string>::const_iterator i;
            for (i = retries.begin(); i != retries.end(); ++i)
                retriesArray.push_front(std::make_pair("", ptree(*i)));
            file.put_child("retries", retriesArray);
        }
    else
        {
            file.put("retries", values[RETRIES]);
        }

    JsonOutput::printArray("job.files", file);
}

MsgPrinter::MsgPrinter(ostream& /*out*/): json(false)
{

}

MsgPrinter::~MsgPrinter()
{

}

} /* namespace server */
} /* namespace fts3 */
