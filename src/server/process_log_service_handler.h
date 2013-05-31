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

#include "server_dev.h"
#include "common/pointers.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <string>
#include "SingleDbInstance.h"
#include "common/logger.h"
#include "common/error.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fstream>
#include "config/serverconfig.h"
#include "definitions.h"
#include <boost/algorithm/string.hpp>
#include "producer_consumer_common.h"
#include <boost/filesystem.hpp>

extern bool stopThreads;

namespace fs = boost::filesystem;

FTS3_SERVER_NAMESPACE_START
using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;
using namespace FTS3_CONFIG_NAMESPACE;


template
<
typename TRAITS
>
class ProcessLogServiceHandler : public TRAITS::ActiveObjectType
{
protected:

    using TRAITS::ActiveObjectType::_enqueue;

public:

    /* ---------------------------------------------------------------------- */

    typedef ProcessLogServiceHandler <TRAITS> OwnType;

    /* ---------------------------------------------------------------------- */

    /** Constructor. */
    ProcessLogServiceHandler
    (
        const std::string& desc = "" /**< Description of this service handler
            (goes to log) */
    ) :
        TRAITS::ActiveObjectType("ProcessLogServiceHandler", desc)
    {

    }

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessLogServiceHandler()
    {
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_p
    (
    )
    {

        boost::function<void() > op = boost::bind(&ProcessLogServiceHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }

protected:
    std::vector<struct message_log> queueMsgRecovery;
    std::vector<struct message_log> messages;

    /* ---------------------------------------------------------------------- */
    void executeTransfer_a()
    {

        std::vector<struct message_log>::const_iterator iter;
        std::vector<struct message_log>::const_iterator iter_restore;

        while (stopThreads==false)   /*need to receive more than one messages at a time*/
            {
                try
                    {

                        if(fs::is_empty(fs::path(LOG_DIR)))
                            {
                                sleep(1);
                                continue;
                            }


                        if(!queueMsgRecovery.empty())
                            {
                                for (iter_restore = queueMsgRecovery.begin(); iter_restore != queueMsgRecovery.end(); ++iter_restore)
                                    {
                                        std::string job = std::string((*iter_restore).job_id).substr(0, 36);
                                        DBSingleton::instance().getDBObjectInstance()->
                                        transferLogFile((*iter_restore).filePath, job, (*iter_restore).file_id, (*iter_restore).debugFile);

                                    }
                                queueMsgRecovery.clear();
                            }

                        runConsumerLog(messages);
                        if(messages.empty())
                            {
                                sleep(1);
                                continue;
                            }
                        else
                            {
                                for (iter = messages.begin(); iter != messages.end(); ++iter)
                                    {
                                        std::string job = std::string((*iter).job_id).substr(0, 36);
                                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Process Log Monitor "
                                                                        << "\nJob id: " << job
                                                                        << "\nFile id: " << (*iter).file_id
                                                                        << "\nLog path: " << (*iter).filePath << commit;
                                        DBSingleton::instance().getDBObjectInstance()->
                                        transferLogFile((*iter).filePath, job , (*iter).file_id, (*iter).debugFile);
                                    }
                                messages.clear();
                            }
                        sleep(1);
                    }
                catch (const fs::filesystem_error& ex)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
                        for (iter_restore = messages.begin(); iter_restore != messages.end(); ++iter_restore)
                            queueMsgRecovery.push_back(*iter_restore);
                        sleep(1);
                    }
                catch (Err& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
                        for (iter_restore = messages.begin(); iter_restore != messages.end(); ++iter_restore)
                            queueMsgRecovery.push_back(*iter_restore);
                        sleep(1);
                    }
                catch (...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message log thrown unhandled exception" << commit;
                        for (iter_restore = messages.begin(); iter_restore != messages.end(); ++iter_restore)
                            queueMsgRecovery.push_back(*iter_restore);
                        sleep(1);
                    }
            }
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

