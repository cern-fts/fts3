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

extern bool stopThreads;

FTS3_SERVER_NAMESPACE_START

template <typename TRAITS>
class HeartBeatHandlerActive: public TRAITS::ActiveObjectType
{
protected:
    using TRAITS::ActiveObjectType::_enqueue;

public:

    HeartBeatHandlerActive(const std::string& desc = ""):
        TRAITS::ActiveObjectType("HeartBeatHandlerActive", desc) {}

    void beat()
    {

        boost::function<void() > op = boost::bind(&HeartBeatHandlerActive::beat_impl, this);
        this->_enqueue(op);
    }

private:

    void beat_impl(void)
    {
        while (!stopThreads)
            {
                try
                    {
                        bool foundRecords = db::DBSingleton::instance().getDBObjectInstance()->updateOptimizer();
			if(!foundRecords) //if there were no records sleep less time to get results if possible
				sleep(30);
			else
                        	sleep(60);
                    }
                catch(std::exception& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread HeartBeatHandlerActive " << e.what() <<  commit;
                        sleep(60);
                    }
                catch(...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread HeartBeatHandlerActive unknown" <<  commit;
                        sleep(60);
                    }
            }
    }
};


//----------------------------
class HeartBeatActive;
struct HeartBeatTraitsActive
{
    typedef ActiveObject <ThreadPool::ThreadPool, Traced<HeartBeatActive> > ActiveObjectType;
};

/* -------------------------------------------------------------------------- */

class HearBeatActive : public HeartBeatHandlerActive <HeartBeatTraitsActive>
{
public:
    explicit HearBeatActive() {}
    virtual ~HearBeatActive() {};
};

FTS3_SERVER_NAMESPACE_END
