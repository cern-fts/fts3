/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include "process_service_reuse_handler.h"

extern bool stopThreads;
extern time_t retrieveRecords;


FTS3_SERVER_NAMESPACE_START
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;
using namespace FTS3_CONFIG_NAMESPACE;

template
<
typename TRAITS
>
class ProcessServiceMultihopHandler : public ProcessServiceReuseHandler<TRAITS>
{

public:

    /* ---------------------------------------------------------------------- */

    typedef ProcessServiceMultihopHandler <TRAITS> OwnType;

    /* ---------------------------------------------------------------------- */

    /** Constructor. */
    ProcessServiceMultihopHandler(std::string const & desc = "") : ProcessServiceReuseHandler<TRAITS>(desc) {}

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessServiceMultihopHandler() {}

    void executeTransfer_p()
    {
        boost::function<void() > op = boost::bind(&ProcessServiceMultihopHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }

protected:

    void executeUrlcopy()
    {
        std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFiles> > > > voQueues;
        DBSingleton::instance().getDBObjectInstance()->getMultihopJobs(voQueues);
        std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFiles> > > >::iterator vo_it;

        bool empty = false;
        int maxUrlCopy = theServerConfig().get<int> ("MaxUrlCopyProcesses");
        int urlCopyCount = countProcessesWithName("fts_url_copy");

        while(!empty)
            {
                empty = true;
                for (vo_it = voQueues.begin(); vo_it != voQueues.end(); ++vo_it)
                    {
                        std::queue< std::pair<std::string, std::list<TransferFiles> > > & vo_jobs = vo_it->second;
                        if (!vo_jobs.empty())
                            {
                                empty = false; //< if we are here there are still some data
                                std::pair< std::string, std::list<TransferFiles> > const job = vo_jobs.front();
                                vo_jobs.pop();
                                if (maxUrlCopy > 0 && urlCopyCount > maxUrlCopy) {
                                    FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                                    << "Reached limitation of MaxUrlCopyProcesses"
                                    << commit;
                                    return;
                                } else {
                                    ProcessServiceReuseHandler<TRAITS>::startUrlCopy(job.first, job.second);
                                    ++urlCopyCount;
                                }
                            }
                    }
            }
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_a()
    {
        static bool drainMode = false;

        while (true)
            {
                retrieveRecords = time(0);

                try
                    {
                        if (stopThreads)
                            {
                                return;
                            }

                        if (DrainMode::getInstance())
                            {
                                if (!drainMode)
                                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more transfers for this instance!" << commit;
                                drainMode = true;
                                sleep(15);
                                continue;
                            }
                        else
                            {
                                drainMode = false;
                            }

                        executeUrlcopy();
                    }
                catch (std::exception& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_multihop_handler " << e.what() << commit;
                        sleep(2);
                    }
                catch (...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_multihop_handler!" << commit;
                        sleep(2);
                    }
                sleep(2);
            } /*end while*/
    }

    /* ---------------------------------------------------------------------- */
    struct TestHelper
    {

        TestHelper()
            : loopOver(false)
        {
        }

        bool loopOver;
    }
    _testHelper;
};

FTS3_SERVER_NAMESPACE_END

