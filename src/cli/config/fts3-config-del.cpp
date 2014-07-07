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
 * fts3-config-del.cpp
 *
 *  Created on: Jul 25, 2012
 *      Author: Michał Simon
 */


#include "GSoapContextAdapter.h"
#include "ui/DelCfgCli.h"

#include "exception/cli_exception.h"

#include <string>
#include <vector>
#include <memory>

using namespace std;
using namespace fts3::cli;

/**
 * This is the entry point for the fts3-config-set command line tool.
 */
int main(int ac, char* av[])
{
	unique_ptr<DelCfgCli> cli(new DelCfgCli);

    try
        {
            // create and initialize the command line utility
            cli->parse(ac, av);
            if (!cli->validate()) return 0;

            // validate command line options, and return respective gsoap context
            GSoapContextAdapter& ctx  = cli->getGSoapContext();

            config__Configuration *config = soap_new_config__Configuration(ctx, -1);
            config->cfg = cli->getConfigurations();

            implcfg__delConfigurationResponse resp;
            ctx.delConfiguration(config, resp);


            cout << "Done, config deleted" << endl;

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
