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
#include "TransferTypes.h"
#include "ui/TransferStatusCli.h"

#include "common/JobStatusHandler.h"

#include <vector>
#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/assign.hpp>

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace fts3::cli;
using namespace fts3::common;

/**
 * This is the entry point for the fts3-transfer-status command line tool.
 */
int main(int ac, char* av[])
{
    static const int DEFAULT_LIMIT = 100;

    scoped_ptr<TransferStatusCli> cli;

    try
        {
            // create and initialize the command line utility
            cli.reset (
                getCli<TransferStatusCli>(ac, av)
            );

            // validate command line options, and return respective gsoap context
            optional<GSoapContextAdapter&> opt = cli->validate();
            if (!opt.is_initialized()) return 0;
            GSoapContextAdapter& ctx = opt.get();

            // get job IDs that have to be check
            vector<string> jobIds = cli->getJobIds();
            // iterate over job IDs
            vector<string>::iterator it;
            for (it = jobIds.begin(); it < jobIds.end(); it++)
                {

                    string jobId = *it;

                    if (cli->isVerbose())
                        {
                            // do the request
                            JobSummary summary = ctx.getTransferJobSummary(jobId);
                            // print the response
                            cli->printer().job_summary(summary);
                        }
                    else
                        {
                            // do the request
                            fts3::cli::JobStatus status = ctx.getTransferJobStatus(jobId);
                            // print the response
                            if (!status.jobStatus.empty())
                                {
                                    cli->printer().status(status);
                                }
                        }

                    // TODO test!
                    // check if the -l option has been used
                    if (cli->list())
                        {
                            int offset = 0;
                            int cnt = 0;

                            do
                                {
                                    // do the request
                                    impltns__getFileStatusResponse resp;
                                    cnt = ctx.getFileStatus(jobId, offset, DEFAULT_LIMIT, resp);

                                    if (cnt > 0 && resp._getFileStatusReturn)
                                        {

                                            std::vector<tns3__FileTransferStatus * >& vect = resp._getFileStatusReturn->item;
                                            std::vector<tns3__FileTransferStatus * >::iterator it;

                                            // print the response
                                            for (it = vect.begin(); it < vect.end(); it++)
                                                {
                                                    tns3__FileTransferStatus* stat = *it;

                                                    vector<string> values =
                                                        list_of
                                                        (*stat->sourceSURL)
                                                        (*stat->destSURL)
                                                        (*stat->transferFileState)
                                                        (lexical_cast<string>(stat->numFailures))
                                                        (*stat->reason)
                                                        (lexical_cast<string>(stat->duration))
                                                        ;

                                                    cli->printer().file_list(values);
                                                }
                                        }

                                    offset += cnt;
                                }
                            while(cnt == DEFAULT_LIMIT);
                        }
                }

        }
    catch(std::exception& ex)
        {
            if (cli.get())
                cli->printer().error_msg(ex.what());
            return 1;
        }
    catch(string& ex)
        {
            if (cli.get())
                cli->printer().gsoap_error_msg(ex);
            return 1;
        }
    catch(...)
        {
            if (cli.get())
                cli->printer().error_msg("Exception of unknown type!");
            return 1;
        }

    return 0;
}
