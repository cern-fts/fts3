/*
 *	Copyright notice:
 *	Copyright Â© Members of the EMI Collaboration, 2010.
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
 * fts3-transfer-del.cpp
 *
 *  Created on: Nov 7, 2013
 *      Author: Christina Skarpathiotaki
 */

#include "GSoapContextAdapter.h"
#include "ProxyCertificateDelegator.h"
#include "MsgPrinter.h"
#include "ui/SrcDelCli.h"
#include <boost/scoped_ptr.hpp>

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "exception/cli_exception.h"
#include "JsonOutput.h"

using namespace boost;
using namespace std;
using namespace fts3::cli;

/**
 * This is the entry point for the fts3-transfer-delete command line tool.
 */




int main(int ac, char* av[])
{
    scoped_ptr<SrcDelCli> cli;

    try
        {
            // create and initialize the command line utility
            cli.reset(
                new SrcDelCli
            );

            cli->SrcDelCli::parse(ac,av);

            // validate command line options, and return respective gsoap context
            if (!cli->validate()) return 1;
            GSoapContextAdapter& ctx = cli->getGSoapContext();

            // delegate Proxy Certificate
            ProxyCertificateDelegator handler (
                cli->getService(),
                "",
                0,
                cli->printer()
            );

            handler.delegate();


            vector<string> vect =  cli->getFileName();

            string resjobid = ctx.deleteFile(vect);
            std::cout<<"Job_id : "<<resjobid <<endl;
                

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
                cli->printer().error_msg("__Exception of unknown type!");
            else
                std::cerr << "~Exception of unknown type!" << std::endl;
            return 1;
        }

    return 0;
}
