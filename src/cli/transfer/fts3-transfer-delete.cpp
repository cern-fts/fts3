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

using namespace fts3::cli;

/**
 * This is the entry point for the fts3-transfer-delete command line tool.
 */

int main(int ac, char* av[])
{
    try
        {
            SrcDelCli cli;
            // create and initialise the command line utility
            cli.parse(ac,av);

            // validate command line options, and return respective gsoap context
            if (!cli.validate()) return 1;

            std::vector<std::string> vect =  cli.getFileName();
            if(vect.size() == 0)
                {
                    std::cout << "You need to provide either a file name of a bulk deletion or a list of files to be deleted" << std::endl;
                    return 1;
                }

            GSoapContextAdapter ctx (cli.getService());
            ctx.printServiceDetails(cli.isVerbose());
            cli.printCliDeatailes();

            // delegate Proxy Certificate
            ProxyCertificateDelegator handler (
                cli.getService(),
                "",
                0
            );

            handler.delegate();

            std::string resjobid = ctx.deleteFile(vect);
            std::cout << resjobid <<endl;


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
