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
 * MsgPrinter.h
 *
 *  Created on: Dec 11, 2012
 *      Author: simonm
 */

#ifndef MSGPRINTER_H_
#define MSGPRINTER_H_

#include "TransferTypes.h"

#include <string>
#include <vector>
#include <map>
#include <iostream>

#include <boost/property_tree/ptree.hpp>

namespace fts3
{
namespace cli
{

using namespace std;
using namespace boost::property_tree;

class MsgPrinter
{

public:

    void delegation_local_expiration(long int h, long int m); //
    void delegation_service_proxy(long int h, long int m); //
    void delegation_msg(string msg); //
    void delegation_request_duration(long int h, long int m); //
    void delegation_request_retry(); //
    void delegation_request_success(bool b); //
    void delegation_request_error(string error); //

    void endpoint(string endpoint); //
    void service_version(string version); //
    void service_interface(string interface); //
    void service_schema(string schema); //
    void service_metadata(string metadata); //
    void client_version(string version); //
    void client_interface(string interface); //


    void version(string version); //

    void bulk_submission_error(int line, string msg); //
    void wrong_endpoint_format(string endpoint); //
    void missing_parameter(string name);
    void error_msg(string msg); //
    void gsoap_error_msg(string msg); //

    void cancelled_job( pair<string, string> id_status);

    void job_id(string job_id); //
    void status(JobStatus js);
    void job_status(JobStatus js);
    void job_summary(JobSummary js);
    void file_list(vector<string> values, vector<string> retries);

    MsgPrinter(ostream& out = std::cout);
    virtual ~MsgPrinter();

    void setVerbose(bool verbose)
    {
        this->verbose = verbose;
    }

    void setJson(bool json)
    {
        this->json = json;
    }


private:

    ptree getItem (map<string, string> values);

    void put (ptree&root, string name, map<string, string>& object);

    void addToArray(ptree& root, string name, map<string, string>& object);

    void addToArray(ptree& root, string name, string value);

    void addToArray(ptree& root, string name, const ptree& node);

    ///
    bool verbose;
    ///
    bool json;
    ///
    ptree json_out;

    /// the output stream
    ostream& out;
};

} /* namespace server */
} /* namespace fts3 */
#endif /* MSGPRINTER_H_ */
