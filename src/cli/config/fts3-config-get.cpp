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

using namespace std;
using namespace fts3::cli;

/**
 * This is the entry point for the fts3-config-set command line tool.
 */
int main(int ac, char* av[])
{

    try
        {
            // create and initialize the command line utility
            std::unique_ptr<GetCfgCli> cli (
                getCli<GetCfgCli>(ac, av)
            );
            if (!cli->validate()) return 0;

            // validate command line options, and return respective gsoap context
            GSoapContextAdapter& ctx = cli->getGSoapContext();

            string source = cli->getSource();
            string destination =  cli->getDestination();

            if (cli->getBandwidth())
                {
                    implcfg__getBandwidthLimitResponse resp;
                    ctx.getBandwidthLimit(resp);

                    cout << resp.limit << std::endl;
                }
            else
                {
                    string all;
                    if (cli->all())
                        {
                            if (!source.empty() && !destination.empty()) throw bad_option("all", "'--all' may only be used if querying for a single SE");
                            all = "all";
                        }

                    if (cli->vo()) all = "vo";

                    implcfg__getConfigurationResponse resp;
                    ctx.getConfiguration(source, destination, all, cli->getName(), resp);

                    vector<string> &cfgs = resp.configuration->cfg;
                    ostream_iterator<string> it(cout, "\n");
                    copy(cfgs.begin(), cfgs.end(), it);

                    if(cfgs.size() == 0)
                      cout << "No config stored into the database" << endl;
                }
        }
    catch(cli_exception const & ex)
        {
            cout << ex.what() << endl;
            return 1;
        }
    catch(std::exception& e)
        {
            cerr << "error: " << e.what() << "\n";
            return 1;
        }
    catch(...)
        {
            cerr << "Exception of unknown type!\n";
            return 1;
        }

    return 0;
}
