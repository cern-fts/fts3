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

    void beat_impl(void)
    {
        while (!stopThreads)
            {
                unsigned index, count, start, end;
                try
                    {
                        db::DBSingleton::instance().getDBObjectInstance()->updateHeartBeat(&index, &count, &start, &end);
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Systole: host " << index << " out of " << count
                                                        << " [" << std::hex << start << ':' << std::hex << end << ']'
                                                        << std::dec
                                                        << commit;
                    }
                catch (const std::exception& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Hearbeat failed: " << e.what() << commit;
                    }
                sleep(60);
            }
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
