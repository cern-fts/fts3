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

#include <algorithm>
#include <sstream>

using namespace fts3::cli;
using namespace fts3::common;


/**
 * This is the entry point for the fts3-transfer-list command line tool.
 */
int main(int ac, char* av[])
{
    try
        {
            ListTransferCli cli;
            // Initialise the command line utility
            cli.parse(ac, av);
            if (!cli.validate()) return 1;

            if (cli.rest())
                {
                    std::vector<std::string> statuses = cli.getStatusArray();
                    std::string dn = cli.getUserDn(), vo = cli.getVoName();

                    std::string url = cli.getService() + "/jobs";

                    // prefix will be holding '?' at the first concatenation and then '&'
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

                    std::string capath = cli.capath();
                    std::string proxy = cli.proxy();

                    std::stringstream ss;

                    ss << "{\"jobs\":";
                    HttpRequest http (url, capath, proxy, ss);
                    http.get();
                    ss << '}';

                    ResponseParser parser(ss);
                    std::vector<fts3::cli::JobStatus> stats = parser.getJobs("jobs");

                    MsgPrinter::instance().print(stats);
                    return 0;
                }

            // validate command line options, and return respective gsoap context
            GSoapContextAdapter ctx (cli.getService());
            ctx.printServiceDetails();
            cli.printCliDeatailes();

            std::vector<std::string> array = cli.getStatusArray();
            std::vector<fts3::cli::JobStatus> statuses =
                ctx.listRequests(array, cli.getUserDn(), cli.getVoName(), cli.source(), cli.destination());

            MsgPrinter::instance().print(statuses);
        }
    catch(cli_exception const & ex)
        {
            MsgPrinter::instance().print(ex);
            return 1;
        }
    catch(std::exception& ex)
        {
            MsgPrinter::instance().print(ex);
            return 1;
        }
    catch(...)
        {
            MsgPrinter::instance().print("error", "exception of unknown type!");
            return 1;
        }

    return 0;
}
