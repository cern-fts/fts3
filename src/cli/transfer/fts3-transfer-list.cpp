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
 * fts3-transfer-list.cpp
 *
 *  Created on: Mar 1, 2012
 *      Author: Michal Simon
 */


#include "GSoapContextAdapter.h"
#include "ui/ListTransferCli.h"
#include "rest/HttpRequest.h"
#include "rest/ResponseParser.h"

#include "common/JobStatusHandler.h"

#include "exception/cli_exception.h"
#include "JsonOutput.h"

#include <boost/scoped_ptr.hpp>

#include <algorithm>
#include <sstream>

using namespace std;
using namespace boost;
using namespace fts3::cli;
using namespace fts3::common;


/**
 * This is the entry point for the fts3-transfer-list command line tool.
 */
int main(int ac, char* av[])
{
    JsonOutput::create();
    scoped_ptr<ListTransferCli> cli;

    try
        {
            // create and initialize the command line utility
            cli.reset(
                getCli<ListTransferCli>(ac, av)
            );
            if (!cli->validate()) return 1;

            if (cli->rest())
                {
                    vector<string> statuses = cli->getStatusArray();
                    string dn = cli->getUserDn(), vo = cli->getVoName();

                    string url = cli->getService() + "/jobs";

                    // prefix will be holding '?' at the first contacenation and then '&'
                    char prefix = '?';

                    if (!dn.empty())
                        {
                            url += prefix;
                            url += "user_dn=";
                            url += dn;
                            prefix = '&';
                        }

                    if (!vo.empty())
                        {
                            url += prefix;
                            url += "vo_name=";
                            url += vo;
                            prefix = '&';
                        }

                    if (!statuses.empty())
                        {
                            url += prefix;
                            url += "job_state=";
                            url += *statuses.begin();
                            prefix = '&';
                        }

                    string capath = cli->capath();
                    string proxy = cli->proxy();

                    stringstream ss;

                    ss << "{\"jobs\":";
                    HttpRequest http (url, capath, proxy, ss);
                    http.get();
                    ss << '}';

                    ResponseParser parser(ss);
                    vector<fts3::cli::JobStatus> stats = parser.getJobs("jobs");

                    cli->printer().print(stats);
                    return 0;
                }

            // validate command line options, and return respective gsoap context
            GSoapContextAdapter& ctx = cli->getGSoapContext();

            vector<string> array = cli->getStatusArray();
            vector<fts3::cli::JobStatus> statuses = ctx.listRequests(array, cli->getUserDn(), cli->getVoName());

            cli->printer().print(statuses);
        }
    catch(cli_exception const & ex)
        {
            if (cli->isJson()) JsonOutput::print(ex);
            else std::cout << ex.what() << std::endl;
            return 1;
        }
    catch(std::exception& ex)
        {
            if (cli.get())
                cli->printer().error_msg(ex.what());
            else
                std::cerr << ex.what() << std::endl;
            return 1;
        }
    catch(...)
        {
            if (cli.get())
                cli->printer().error_msg("Exception of unknown type!");
            else
                std::cerr << "Exception of unknown type!" << std::endl;
            return 1;
        }

    return 0;
}
