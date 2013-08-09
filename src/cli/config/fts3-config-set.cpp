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
 * fts3-config-set.cpp
 *
 *  Created on: Apr 3, 2012
 *      Author: Michał Simon
 */


#include "GSoapContextAdapter.h"
#include "ui/SetCfgCli.h"
#include "common/error.h"

#include <string>
#include <vector>
#include <memory>

using namespace std;
using namespace fts3::cli;
using namespace fts3::common;

/**
 * This is the entry point for the fts3-config-set command line tool.
 */
int main(int ac, char* av[])
{

    try
        {
            // create and initialize the command line utility
            auto_ptr<SetCfgCli> cli (
                getCli<SetCfgCli>(ac, av)
            );

            // validate command line options, and return respective gsoap context
            optional<GSoapContextAdapter&> opt = cli->validate();

            if (!opt.is_initialized()) return 0;

            GSoapContextAdapter& ctx = opt.get();

            optional<bool> drain = cli->drain();
            if (drain.is_initialized())
                {
                    ctx.doDrain(drain.get());
                }

            optional<int> retry = cli->retry();
            if (retry.is_initialized())
                {
                    ctx.retrySet(*retry);
                }

            optional<int> mode = cli->optimizer_mode();
            if (mode.is_initialized())
                {
                    ctx.optimizerModeSet(*mode);
                }

            optional<unsigned> queueTimeout = cli->queueTimeout();
            if (queueTimeout.is_initialized())
                {
                    ctx.queueTimeoutSet(*queueTimeout);
                }

            map<string, int> bring_online = cli->getBringOnline();
            if (!bring_online.empty())
                {
                    ctx.setBringOnline(bring_online);
                    // if bring online was used normal config was not!
                    return 0;
                }

            config__Configuration *config = soap_new_config__Configuration(ctx, -1);
            config->cfg = cli->getConfigurations();
            if (config->cfg.empty()) return 0;

            implcfg__setConfigurationResponse resp;
            ctx.setConfiguration(config, resp);

        }
    catch(string& ex)
        {
            cout << ex << endl;
            return 1;
        }
    catch(Err& ex)
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
