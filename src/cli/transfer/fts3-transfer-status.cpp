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

#include "rest/ResponseParser.h"
#include "rest/HttpRequest.h"
#include "exception/bad_option.h"

#include "common/JobStatusHandler.h"

#include "exception/cli_exception.h"
#include "JsonOutput.h"

#include <algorithm>
#include <vector>
#include <string>
#include <iterator>

#include <boost/scoped_ptr.hpp>
#include <boost/assign.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace fts3::cli;
using namespace fts3::common;

static bool isTransferFailed(const std::string& state)
{
    return state == "CANCELED" || state == "FAILED";
}

/**
 * This is the entry point for the fts3-transfer-status command line tool.
 */
int main(int ac, char* av[])
{
    static const int DEFAULT_LIMIT = 100;

    JsonOutput::create();
    scoped_ptr<TransferStatusCli> cli (new TransferStatusCli);

    try
        {
            // create and initialize the command line utility
            cli->parse(ac, av);
            if (!cli->validate()) return 0;

            if (cli->rest())
                {
                    vector<string> jobIds = cli->getJobIds();
                    vector<string>::iterator itr;

                    for (itr = jobIds.begin(); itr != jobIds.end(); ++itr)
                        {
                            string url = cli->getService() + "/jobs/" + *itr;

                            stringstream ss;
                            HttpRequest http (url, cli->capath(), cli->proxy(), ss);
                            http.get();
                            ResponseParser respons(ss);

                            cout << respons.get("job_state") << endl;

                        }
                    return 0;
                }

            // validate command line options, and return respective gsoap context
            GSoapContextAdapter& ctx = cli->getGSoapContext();

            if (cli->p())
                {
                    vector<string> ids = cli->getJobIds();
                    vector<string>::const_iterator it;

                    for (it = ids.begin(); it != ids.end(); ++it)
                        {
                            tns3__DetailedJobStatus* ptr = ctx.getDetailedJobStatus(*it);
                            cli->printer().print(*it, ptr->transferStatus);
                        }

                    return 0;
                }

            // archived content?
            bool archive = cli->queryArchived();
            // get job IDs that have to be check
            vector<string> jobIds = cli->getJobIds();

            // iterate over job IDs
            vector<string>::iterator it;
            for (it = jobIds.begin(); it < jobIds.end(); it++)
                {

                    string jobId = *it;

                    std::ofstream failedFiles;
                    if (cli->dumpFailed())
                        {
                            failedFiles.open(jobId.c_str(), ios_base::out);
                            if (failedFiles.fail())
                                throw bad_option("dump-failed", strerror(errno));
                        }

                    if (cli->isVerbose())
                        {
                            // do the request
                            JobSummary summary = ctx.getTransferJobSummary(jobId, archive);
                            // print the response
                            cli->printer().job_summary(summary);
                        }
                    else
                        {
                            // do the request
                            fts3::cli::JobStatus status = ctx.getTransferJobStatus(jobId, archive);
                            // print the response
                            if (!status.jobStatus.empty())
                                {
                                    cli->printer().status(status);
                                }
                        }

                    // TODO test!
                    // If a list is requested, or dumping the failed transfers,
                    // get the transfers
                    if (cli->list() || cli->dumpFailed())
                        {
                            int offset = 0;
                            int cnt = 0;

                            do
                                {
                                    // do the request
                                    impltns__getFileStatusResponse resp;
                                    cnt = ctx.getFileStatus(jobId, archive, offset, DEFAULT_LIMIT, cli->detailed(), resp);

                                    if (cnt > 0 && resp._getFileStatusReturn)
                                        {

                                            std::vector<tns3__FileTransferStatus * >& vect = resp._getFileStatusReturn->item;
                                            std::vector<tns3__FileTransferStatus * >::iterator it;
                                            std::vector<tns3__FileTransferRetry*>::const_iterator ri;

                                            // print the response
                                            for (it = vect.begin(); it < vect.end(); it++)
                                                {
                                                    tns3__FileTransferStatus* stat = *it;

                                                    if (cli->list())
                                                        {
                                                            vector<string> values =
                                                                list_of
                                                                (*stat->sourceSURL)
                                                                (*stat->destSURL)
                                                                (*stat->transferFileState)
                                                                (lexical_cast<string>(stat->numFailures))
                                                                (*stat->reason)
                                                                (lexical_cast<string>(stat->duration))
                                                                ;

                                                            vector<string> retries;
                                                            transform(
                                                                stat->retries.begin(),
                                                                stat->retries.end(),
                                                                inserter(retries, retries.begin()),
                                                                lambda::bind(&tns3__FileTransferRetry::reason, lambda::_1)
                                                            );

                                                            cli->printer().file_list(values, retries);
                                                        }

                                                    if (cli->dumpFailed() && isTransferFailed(*stat->transferFileState))
                                                        {
                                                            failedFiles << *stat->sourceSURL << " "
                                                                        << *stat->destSURL
                                                                        << std::endl;
                                                        }
                                                }
                                        }

                                    offset += cnt;
                                }
                            while(cnt == DEFAULT_LIMIT);
                        }
                }

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
                cli->printer().error_msg("Exception of unknown type!");
            else
                std::cerr << "Exception of unknown type!" << std::endl;
            return 1;
        }

    return 0;
}
