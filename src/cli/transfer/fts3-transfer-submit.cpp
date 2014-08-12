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
 */

#include "GSoapContextAdapter.h"
#include "ProxyCertificateDelegator.h"
#include "File.h"

#include "ui/SubmitTransferCli.h"
#include "rest/HttpRequest.h"
#include "common/JobStatusHandler.h"
#include "exception/cli_exception.h"


using namespace fts3::cli;
using namespace fts3::common;

/**
 * This is the entry point for the fts3-transfer-submit command line tool.
 */
int main(int ac, char* av[])
{
    try
        {
            SubmitTransferCli cli;
            cli.parse(ac, av);
            if (!cli.validate()) return 1;

            if (cli.rest())
                {
                    std::string url = cli.getService() + "/jobs";
                    std::string job = cli.getFileName();

                    HttpRequest http (url, cli.capath(), cli.proxy(), cout);
                    http.put(job);
                    return 0;
                }

            // validate command line options, and return respective gSOAP context
            GSoapContextAdapter ctx (cli.getService());
            ctx.printServiceDetails();
            cli.printCliDeatailes();

            std::string jobId("");

            std::vector<File> files = cli.getFiles();


            std::map<std::string, std::string> params = cli.getParams();

            // delegate Proxy Certificate
            ProxyCertificateDelegator handler (
                cli.getService(),
                cli.getDelegationId(),
                cli.getExpirationTime()
            );

            handler.delegate();

            // submit the job
            jobId = ctx.transferSubmit (
                        files,
                        params
                    );

            MsgPrinter::instance().print("", "job_id", jobId);

            // check if the -b option has been used
            if (cli.isBlocking())
                {
                    std::string status;
                    // wait until the transfer is ready
                    do
                        {
                            sleep(2);
                            status = ctx.getTransferJobStatus(jobId, false).getStatus();
                        }
                    while (!JobStatusHandler::getInstance().isTransferFinished(status));
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
