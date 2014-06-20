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
 * fts3-config-get.cpp
 *
 *  Created on: Apr 3, 2012
 *      Author: Michał Simon
 */

#include "GSoapContextAdapter.h"
#include "MsgPrinter.h"
#include "ui/CancelCli.h"
#include "rest/HttpRequest.h"
#include "rest/ResponseParser.h"

#include "common/JobStatusHandler.h"

#include "exception/cli_exception.h"
#include "JsonOutput.h"

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

#include <boost/scoped_ptr.hpp>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

using namespace boost;
using namespace std;
using namespace fts3::cli;
using namespace fts3::common;

/**
 * This is the entry point for the fts3-transfer-cancel command line tool.
 */
int main(int ac, char* av[])
{
    JsonOutput::create();
    scoped_ptr<CancelCli> cli;

    try
        {
            // create and initialize the command line utility
            cli.reset(
                getCli<CancelCli>(ac, av)
            );
            if (!cli->validate()) return 0;

            if (cli->rest())
                {
                    vector<string> jobIds = cli->getJobIds();
                    vector<string>::iterator itr;

                    std::vector< std::pair< std::string, std::string> > ret;

                    for (itr = jobIds.begin(); itr != jobIds.end(); ++itr)
                        {
                            std::stringstream ss;
                            string url = cli->getService() + "/jobs/" + *itr;
                            HttpRequest http (url, cli->capath(), cli->proxy(), ss);
                            http.del();

                            ResponseParser p(ss);
                            ret.push_back(std::make_pair(p.get("job_id"), p.get("job_state")));
                        }

                    cli->printer().print(ret);

                    return 0;
                }

            // validate command line options, and return respective gsoap context
            GSoapContextAdapter& ctx = cli->getGSoapContext();

            vector<string> jobs = cli->getJobIds();

            if (jobs.empty())
                {
                    cli->printer().missing_parameter("Job ID");
                    return 0;
                }

            vector< pair<string, string> > ret = ctx.cancel(jobs);
            cli->printer().print(ret);
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
