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

void MsgPrinter::print_info(std::string const & ostr_subject, std::string const & json_subject, bool flag)
{
    if (!verbose) return;

    if (!json)
        {
            if (flag) (*ostr) << ostr_subject << endl;
            return;
        }

    std::stringstream ss;
    ss << std::boolalpha << flag;

    jout.print(json_subject, ss.str());
}

void MsgPrinter::print_info(std::string const & ostr_subject, std::string const & json_subject, long int h, long int m)
{
    if (!verbose) return;

    if (!json)
        {
            (*ostr) << ostr_subject << ": " << h << "hours and " << m << " minutes." << endl;
            return;
        }

    jout.print(json_subject, lexical_cast<string>(h) + ":" + lexical_cast<string>(m));
}

void MsgPrinter::print_info(std::string const & ostr_subject, std::string const & json_subject, std::string const & msg)
{
    if (!verbose) return;

    print(ostr_subject, json_subject, msg);
}

void MsgPrinter::print_info(std::string const & json_subject, std::string const & msg)
{
    print_info("", json_subject, msg);
}

void MsgPrinter::print_ostr(std::pair<std::string, std::string> const & id_status)
{
    (*ostr) << "job " << id_status.first << ": " << id_status.second << endl;
}

void MsgPrinter::print_json(std::pair<std::string, std::string> const & id_status)
{
    std::map<std::string, std::string> m = boost::assign::map_list_of ("job_id", id_status.first) ("job_state", id_status.second);
    jout.printArray("job", m);
}

void MsgPrinter::print(std::string const & ostr_subject, std::string const & json_subject, std::string const & msg)
{
    if (!json)
        {
            if (!ostr_subject.empty()) (*ostr) << ostr_subject << " : ";
            (*ostr) << msg << std::endl;
        }
    else jout.print(json_subject, msg);
}

void MsgPrinter::print(std::string const & subject, std::string const & msg)
{
    print(subject, subject, msg);
}

void MsgPrinter::print_ostr(JobStatus const & j)
{
    (*ostr) << "Request ID: " << j.jobId << endl;
    (*ostr) << "Status: " << j.jobStatus << endl;

    // if not verbose return
    if (!verbose) return;

    (*ostr) << "Client DN: " << j.clientDn << endl;
    (*ostr) << "Reason: " << (j.reason.empty() ? "<None>": j.reason) << endl;
    (*ostr) << "Submission time: " << j.submitTime << endl;
    (*ostr) << "Files: " << (j.numFiles == -1 ? "n/a" : boost::lexical_cast<std::string>(j.numFiles)) << endl;
    (*ostr) << "Priority: " << j.priority << endl;
    (*ostr) << "VOName: " << j.voName << endl;
    (*ostr) << endl;
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

    jout.printArray("job", object);
}

void MsgPrinter::status(JobStatus js)
{

    if (!json)
        {
            (*ostr) << js.jobStatus << endl;
            return;
        }

    map<string, string> object = map_list_of ("job_id", js.jobId) ("status", js.jobStatus);
    jout.printArray("job", object);
}

void MsgPrinter::job_summary(JobSummary js)
{

    if (!json)
        {
            print_ostr(js.status);
            (*ostr) << "\tActive: " << js.numActive << endl;
            (*ostr) << "\tReady: " << js.numReady << endl;
            (*ostr) << "\tCanceled: " << js.numCanceled << endl;
            (*ostr) << "\tFinished: " << js.numFinished << endl;
            (*ostr) << "\tSubmitted: " << js.numSubmitted << endl;
            (*ostr) << "\tFailed: " << js.numFailed << endl;
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

    jout.printArray("job", object);
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
            (*ostr) << "  Source:      " << values[SOURCE] << endl;
            (*ostr) << "  Destination: " << values[DESTINATION] << endl;
            (*ostr) << "  State:       " << values[STATE] << endl;;
            (*ostr) << "  Reason:      " << values[REASON] << endl;
            (*ostr) << "  Duration:    " << values[DURATION] << endl;

            if (retries.size() > 0)
                {
                    (*ostr) << "  Retries: " << endl;
                    for_each(retries.begin(), retries.end(), (*ostr) << ("    " + lambda::_1) << '\n');
                }
            else
                {
                    (*ostr) << "  Retries:     " << values[RETRIES] << endl;
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

    jout.printArray("job.files", file);
}

void MsgPrinter::print(std::string job_id, std::vector<tns3__DetailedFileStatus *> const & v)
{
    // make sure the vector is not empty
    if (v.empty()) return;
    // create the JSON object
    ptree object;
    // add job ID
    object.put("job_id", job_id);
    // create array element
    pt::ptree array;
    // add each file status to array
    std::vector<tns3__DetailedFileStatus *>::const_iterator it;
    for (it = v.begin(); it != v.end(); ++it)
        {
            tns3__DetailedFileStatus& stat = **it;
            ptree item;
            item.put("file_id", boost::lexical_cast<std::string>(stat.fileId));
            item.put("state", stat.fileState);
            item.put("source_surl", stat.sourceSurl);
            item.put("dest_surl", stat.destSurl);
            array.push_back(std::make_pair("", item));
        }
    // add the array to the JSON object
    object.put_child("files", array);
    // print the JSON object
    jout.printArray("jobs", object);
}

void MsgPrinter::print(cli_exception const & ex)
{
    if (!json) (*ostr) << ex.what() << std::endl;
    else jout.print(ex);
}

void MsgPrinter::print(std::exception const & ex)
{
    if (!json) (*ostr) << ex.what() << std::endl;
}

} /* namespace server */
} /* namespace fts3 */
