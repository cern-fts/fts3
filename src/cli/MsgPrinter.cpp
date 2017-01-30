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

#include "MsgPrinter.h"

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

void MsgPrinter::print_info(std::string const & ostr_subject, std::string const & json_subject, bool flag)
{
    if (!verbose) return;

    if (!json)
        {
            if (flag) (*ostr) << ostr_subject << std::endl;
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
            (*ostr) << ostr_subject << ": " << h << "hours and " << m << " minutes." << std::endl;
            return;
        }

    jout.print(json_subject, boost::lexical_cast<std::string>(h) + ":" + boost::lexical_cast<std::string>(m));
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
    (*ostr) << "job " << id_status.first << ": " << id_status.second << std::endl;
}

void MsgPrinter::print_json(std::pair<std::string, std::string> const & id_status)
{
    std::map<std::string, std::string> m = boost::assign::map_list_of ("job_id", id_status.first) ("job_state", id_status.second);
    jout.printArray("job", m);
}

void MsgPrinter::print_ostr(std::pair<std::string, int> const & keyval)
{
    (*ostr) << keyval.first << ": " << keyval.second << std::endl;
}

void MsgPrinter::print_json(std::pair<std::string, int> const & keyval)
{
    jout.print(keyval.first, keyval.second);
}

void MsgPrinter::print(std::string const & ostr_subject, std::string const & json_subject, std::string const & msg)
{
    if (!json)
        {
            if (!ostr_subject.empty()) (*ostr) << ostr_subject << " : ";
            (*ostr) << msg << std::endl;
        }
    else
        {
            try {
                jout.print(json_subject, msg);
            }
            catch (...) {
                json = false;
                if (!ostr_subject.empty()) (*ostr) << ostr_subject << " : ";
                (*ostr) << msg << std::endl;
            }
        }
}

void MsgPrinter::print(std::string const & subject, std::string const & msg)
{
    print(subject, subject, msg);
}

void MsgPrinter::print(std::string job_id, std::vector<DetailedFileStatus> const & v)
{
    // make sure the vector is not empty
    if (v.empty()) return;
    // create the JSON object
    pt::ptree object;
    // add job ID
    object.put("job_id", job_id);
    // create array element
    pt::ptree array;
    // add each file status to array
    std::vector<DetailedFileStatus>::const_iterator it;
    for (it = v.begin(); it != v.end(); ++it)
        {
            pt::ptree item;
            item.put("file_id", boost::lexical_cast<std::string>(it->fileId));
            item.put("state", it->state);
            item.put("source_surl", it->src);
            item.put("dest_surl", it->dst);
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

void MsgPrinter::print(JobStatus const & status)
{
    if (json) print_json(status);
    else print_ostr(status, true);
}

void MsgPrinter::print_ostr(JobStatus const & j)
{
    print_ostr(j, false);
}

void MsgPrinter::print_ostr(JobStatus const & status, bool short_out)
{
    if (short_out && !verbose)
        {
            (*ostr) << status.status << std::endl;
        }
    else
        {
            (*ostr) << "Request ID: " << status.jobId << std::endl;
            (*ostr) << "Status: " << status.status << std::endl;
        }

    if (verbose)
        {

            (*ostr) << "Client DN: " << status.dn << std::endl;
            (*ostr) << "Reason: " << (status.reason.empty() ? "<None>": status.reason) << std::endl;
            (*ostr) << "Submission time: " << status.submitTime << std::endl;
            (*ostr) << "Files: " << (status.nbFiles == -1 ? "n/a" : boost::lexical_cast<std::string>(status.nbFiles)) << std::endl;
            (*ostr) << "Priority: " << status.priority << std::endl;
            (*ostr) << "VOName: " << status.vo << std::endl;

            if (status.summary.is_initialized())
                {
                    (*ostr) << "\tActive: " << std::get<0>(*status.summary) << std::endl;
                    (*ostr) << "\tReady: " << std::get<1>(*status.summary) << std::endl;
                    (*ostr) << "\tCanceled: " << std::get<2>(*status.summary) << std::endl;
                    (*ostr) << "\tFinished: " << std::get<3>(*status.summary) << std::endl;
                    (*ostr) << "\tSubmitted: " << std::get<4>(*status.summary) << std::endl;
                    (*ostr) << "\tFailed: " << std::get<5>(*status.summary) << std::endl;
                    (*ostr) << "\tStaging: " << std::get<6>(*status.summary) << std::endl;
                    (*ostr) << "\tStarted: " << std::get<7>(*status.summary) << std::endl;
                    (*ostr) << "\tDelete: " << std::get<8>(*status.summary) << std::endl;
                }
        }

    std::vector<FileInfo>::const_iterator it;
    for (it = status.files.begin(); it != status.files.end(); ++it)
        {
            (*ostr) << std::endl;
            (*ostr) << "  Source:      " << it->src << std::endl;
            (*ostr) << "  Destination: " << it->dst << std::endl;
            (*ostr) << "  State:       " << it->state << std::endl;;
            (*ostr) << "  Reason:      " << it->reason << std::endl;
            (*ostr) << "  Duration:    " << it->duration << std::endl;

            if (it->stagingDuration >= 0)
                (*ostr) << "  Staging:     " << it->stagingDuration << std::endl;

            if (it->retries.size() > 0)
                {
                    (*ostr) << "  Retries: " << std::endl;
                    std::for_each(it->retries.begin(), it->retries.end(), (*ostr) << ("    " + boost::lambda::_1) << '\n');
                }
            else
                {
                    (*ostr) << "  Retries:     " << it->nbFailures << std::endl;
                }
        }

    (*ostr) << std::endl;
}

void MsgPrinter::print_json(JobStatus const & status)
{
    pt::ptree job;
    job.put("job_id", status.jobId);
    job.put("status", status.status);

    if (verbose)
        {
            job.put("dn", status.dn);
            job.put("reason", status.reason.empty() ? "<None>": status.reason);
            job.put("submision_time", status.submitTime);
            job.put("file_count", boost::lexical_cast<std::string>(status.nbFiles));
            job.put("priority", boost::lexical_cast<std::string>(status.priority));
            job.put("vo", status.vo);

            if (status.summary.is_initialized())
                {
                    job.put("summary.active", boost::lexical_cast<std::string>(std::get<0>(*status.summary)));
                    job.put("summary.ready", boost::lexical_cast<std::string>(std::get<1>(*status.summary)));
                    job.put("summary.canceled", boost::lexical_cast<std::string>(std::get<2>(*status.summary)));
                    job.put("summary.finished", boost::lexical_cast<std::string>(std::get<3>(*status.summary)));
                    job.put("summary.submitted", boost::lexical_cast<std::string>(std::get<4>(*status.summary)));
                    job.put("summary.failed", boost::lexical_cast<std::string>(std::get<5>(*status.summary)));
                    job.put("summary.staging", boost::lexical_cast<std::string>(std::get<6>(*status.summary)));
                    job.put("summary.started", boost::lexical_cast<std::string>(std::get<7>(*status.summary)));
                    job.put("summary.delete", boost::lexical_cast<std::string>(std::get<8>(*status.summary)));
                }
        }

    if (!status.files.empty())
        {
            pt::ptree files;
            std::vector<FileInfo>::const_iterator it;
            for (it = status.files.begin(); it != status.files.end(); ++it)
                {
                    pt::ptree file;
                    file.put("source", it->src);
                    file.put("destination", it->dst);
                    file.put("state", it->state);
                    file.put("reason", it->reason);
                    file.put("duration", it->duration);

                    if (it->stagingDuration >= 0)
                        file.put("staging", it->stagingDuration);

                    if (it->retries.empty())
                        {
                            file.put("retries", it->nbFailures);
                        }
                    else
                        {
                            pt::ptree retriesArray;
                            std::vector<std::string>::const_iterator i;
                            for (i = it->retries.begin(); i != it->retries.end(); ++i)
                                retriesArray.push_back(std::make_pair("", pt::ptree(*i)));
                            file.put_child("retries", retriesArray);
                        }
                    files.push_back(std::make_pair("", file));
                }

            job.put_child("files", files);
        }

    jout.printArray("job", job);
}

} /* namespace server */
} /* namespace fts3 */
