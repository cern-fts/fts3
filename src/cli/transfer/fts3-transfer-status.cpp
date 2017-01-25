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

#include "RestContextAdapter.h"

#include "JobStatus.h"
#include "ui/TransferStatusCli.h"

#include "rest/ResponseParser.h"
#include "rest/HttpRequest.h"
#include "exception/bad_option.h"

#include "exception/cli_exception.h"
#include "JsonOutput.h"

#include <algorithm>
#include <vector>
#include <string>
#include <iterator>

#include <boost/assign.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

using namespace fts3::cli;

static bool isTransferFailed(const std::string& state)
{
    return state == "CANCELED" || state == "FAILED";
}

/**
 * This is the entry point for the fts3-transfer-status command line tool.
 */
int main(int ac, char* av[])
{
    static const size_t DEFAULT_LIMIT = 10000;

    try
        {
            TransferStatusCli cli;
            // create and initialise the command line utility
            cli.parse(ac, av);
            if (cli.printHelp()) return 0;
            cli.validate();

            RestContextAdapter ctx(cli.getService(), cli.capath(), cli.proxy(), cli.isInsecure());

            cli.printApiDetails(ctx);

            // archived content?
            bool archive = cli.queryArchived();
            // get job IDs that have to be check
            std::vector<std::string> jobIds = cli.getJobIds();

            // iterate over job IDs
            std::vector<std::string>::iterator it;
            for (it = jobIds.begin(); it < jobIds.end(); it++)
                {

                    std::string jobId = *it;

                    std::ofstream failedFiles;
                    if (cli.dumpFailed())
                        {
                            failedFiles.open(jobId.c_str(), std::ios_base::out);
                            if (failedFiles.fail())
                                throw bad_option("dump-failed", strerror(errno));
                        }

                    JobStatus status = cli.isVerbose() ?
                                       ctx.getTransferJobSummary(jobId, archive)
                                       :
                                       ctx.getTransferJobStatus(jobId, archive)
                                       ;

                    // If a list is requested, or dumping the failed transfers,
                    // get the transfers
                    if (cli.list() || cli.dumpFailed())
                        {
                            size_t offset = 0;
                            size_t cnt = 0;

                            do
                                {
                                    // do the request
                                    std::vector<FileInfo> const vect =
                                        ctx.getFileStatus(jobId, archive, (int)offset, (int)DEFAULT_LIMIT, cli.detailed());

                                    cnt = vect.size();

                                    std::vector<FileInfo>::const_iterator it;
                                    for (it = vect.begin(); it < vect.end(); it++)
                                        {
                                            if (cli.list())
                                                {
                                                    status.addFile(*it);
                                                }
                                            else if (cli.dumpFailed() && isTransferFailed(it->getState()))
                                                {
                                                    failedFiles << it->getSource() << " " << it->getDestination() << std::endl;
                                                }
                                        }

                                    offset += vect.size();
                                }
                            while(cnt == DEFAULT_LIMIT);
                        }

                    MsgPrinter::instance().print(status);
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
