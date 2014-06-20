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

#include "ws-ifce/gsoap/gsoap_stubs.h"

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
    void error_msg(string msg); //

    template<typename T>
    void print(std::vector<T> const & v);
    void print(std::string job_id, std::vector<tns3__DetailedFileStatus*> const & v);

    void job_id(string job_id); //
    void status(JobStatus js);
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

    static void print_cout(std::pair<std::string, std::string> const & id_status);
    static void print_json(std::pair<std::string, std::string> const & id_status);

    static void print_cout(JobStatus const & j);
    static void print_json(JobStatus const & j);

    template<typename T>
    static void print_cout() {}

    template<typename T>
    static void print_json() {}

    ///
    static bool verbose;
    ///
    bool json;
};

template<typename T>
void MsgPrinter::print(std::vector<T> const & v)
{
	if (v.empty())
	{
		if (json) print_json<T>();
		else print_cout<T>();
		return;
	}

    void (*print)(T const &);

    if (json) print = print_json;
    else print = print_cout;

    std::for_each(v.begin(), v.end(), print);
}

template<>
void MsgPrinter::print_cout<JobStatus>();

template<>
void MsgPrinter::print_json<JobStatus>();

} /* namespace server */
} /* namespace fts3 */

#endif /* MSGPRINTER_H_ */
