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
#include "ui/GetCfgCli.h"
#include "exception/cli_exception.h"
#include "exception/bad_option.h"

#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <iterator>
#include <algorithm>

using namespace fts3::cli;

/**
 * This is the entry point for the fts3-config-set command line tool.
 */
int main(int ac, char* av[])
{
    try
        {
            GetCfgCli cli;
            // create and initialise the command line utility
            cli.parse(ac, av);
            if (!cli.validate()) return 1;

            // validate command line options, and return respective gsoap context
            GSoapContextAdapter ctx (cli.getService());
            cli.printApiDetails(ctx);

            std::string source = cli.getSource();
            std::string destination =  cli.getDestination();

            if (cli.getBandwidth())
                {
                    implcfg__getBandwidthLimitResponse resp;
                    ctx.getBandwidthLimit(resp);

                    std::cout << resp.limit << std::endl;
                }
            else
                {
                    std::string all;
                    if (cli.all())
                        {
                            if (!source.empty() && !destination.empty()) throw bad_option("all", "'--all' may only be used if querying for a single SE");
                            all = "all";
                        }

                    if (cli.vo()) all = "vo";

                    implcfg__getConfigurationResponse resp;
                    ctx.getConfiguration(source, destination, all, cli.getName(), resp);

                    std::vector<std::string> &cfgs = resp.configuration->cfg;
                    std::ostream_iterator<std::string> it(std::cout, "\n");
                    std::copy(cfgs.begin(), cfgs.end(), it);

                    if(cfgs.size() == 0)
                        std::cout << "No config stored into the database" << std::endl;
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
