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

#include "ServiceAdapterFallbackFacade.h"

#include "MsgPrinter.h"
#include "ui/CancelCli.h"
#include "rest/HttpRequest.h"
#include "rest/ResponseParser.h"

#include "exception/cli_exception.h"
#include "JsonOutput.h"

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

using namespace fts3::cli;

/**
 * This is the entry point for the fts3-transfer-cancel command line tool.
 */
int main(int ac, char* av[])
{
    try
        {
            CancelCli cli;
            // create and initialise the command line utility
            cli.parse(ac, av);
            if (cli.printHelp()) return 0;
            cli.validate();

            ServiceAdapterFallbackFacade ctx(cli.getService(), cli.capath(), cli.proxy());

            // print API details if verbose
            cli.printApiDetails(ctx);

            if (!cli.cancelAll())
                {
                    std::vector<std::string> jobs = cli.getJobIds();

                    if (jobs.empty()) throw bad_option("jobid", "missing parameter");

                    std::vector< std::pair<std::string, std::string> > ret = ctx.cancel(jobs);
                    MsgPrinter::instance().print(ret);
                }
            else
                {
                    std::cerr << "Do you really want to cancel all transfer?" << std::endl;
                    std::cerr << "Type 'yes': ";

                    std::string answer;
                    std::cin >> answer;

                    if (answer == "yes")
                        {
                            boost::tuple<int, int> cancelCount = ctx.cancelAll(cli.getVoName());
                            std::vector<std::pair<std::string, int> > result;
                            int canceledJobs = boost::get<0>(cancelCount);
                            int canceledFiles = boost::get<1>(cancelCount);
                            result.push_back(std::make_pair(std::string("jobs"), canceledJobs));
                            result.push_back(std::make_pair(std::string("files"), canceledFiles));
                            MsgPrinter::instance().print(result);
                        }
                    else
                        {
                            std::cout << "Abort!" << std::endl;
                        }
                }
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
