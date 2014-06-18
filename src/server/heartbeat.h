/* Copyright @ Members of the EMI Collaboration, 2013.
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

#include "active_object.h"
#include "SingleDbInstance.h"
#include "threadpool.h"
#include "DrainMode.h"
#include <ctime>

extern bool stopThreads;
extern time_t retrieveRecords;
extern time_t updateRecords;
extern time_t stallRecords;


FTS3_SERVER_NAMESPACE_START

template <typename TRAITS>
class HeartBeatHandler: public TRAITS::ActiveObjectType
{
protected:
    using TRAITS::ActiveObjectType::_enqueue;

public:

    HeartBeatHandler(const std::string& desc = ""):
        TRAITS::ActiveObjectType("HeartBeatHandler", desc) {}

    void beat()
    {

        boost::function<void() > op = boost::bind(&HeartBeatHandler::beat_impl, this);
        this->_enqueue(op);
    }

private:

    bool criticalThreadExpired(time_t retrieveRecords, time_t updateRecords ,time_t stallRecords)
    {
        double diffTime  = 0.0;

        diffTime = std::difftime(std::time(NULL), retrieveRecords);
        if(diffTime > 7200)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Wall time passed retrieve records: " << diffTime << " secs "<< commit;
                return true;
            }

        diffTime = std::difftime(std::time(NULL), updateRecords);
        if(diffTime > 7200)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Wall time passed update records: " << diffTime << " secs "<< commit;
                return true;
            }

        diffTime = std::difftime(std::time(NULL), stallRecords);
        if(diffTime > 10000)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Wall time passed stallRecords and cancelation thread exited: " << diffTime << " secs "<< commit;
                return true;
            }

        return false;
    }


    void beat_impl(void)
    {
        while (!stopThreads)
            {
                try
                    {
                        //if we drain a host, we need to let the other hosts know about it, hand-over all files to the rest
                        if (DrainMode::getInstance())
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more transfers for this instance!" << commit;
                                sleep(1);
                                continue;
                            }

                        if (true == criticalThreadExpired(retrieveRecords, updateRecords , stallRecords))
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "One of the critical threads looks stalled" << commit;
                                // Note: Would be nice to get the pstack output in the log
                                ordered_shutdown();
                            }
                        else
                            {
                                unsigned index=0, count=0, start=0, end=0;
                                std::string serviceName = "fts_server";

                                db::DBSingleton::instance().getDBObjectInstance()->updateHeartBeat(&index, &count, &start, &end, serviceName);
                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Systole: host " << index << " out of " << count
                                                                << " [" << start << ':' << end << ']'
                                                                << commit;
                            }
                        sleep(60);
                    }
                catch (const std::exception& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Hearbeat failed: " << e.what() << commit;
                        sleep(60);
                    }
                catch (...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Hearbeat failed " << commit;
                        sleep(60);
                    }
            }
    }

    void ordered_shutdown()
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Stopping other threads..." << commit;
        // Give other threads a chance to finish gracefully
        stopThreads = true;
        sleep(30);

        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exiting" << commit;
        _exit(1);
    }
};


//----------------------------
class HeartBeat;
struct HeartBeatTraits
{
    typedef ActiveObject <ThreadPool::ThreadPool, Traced<HeartBeat> > ActiveObjectType;
};

/* -------------------------------------------------------------------------- */

class HearBeat : public HeartBeatHandler <HeartBeatTraits>
{
public:
    explicit HearBeat() {}
    virtual ~HearBeat() {};
};

FTS3_SERVER_NAMESPACE_END
