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

#pragma once

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/tss.hpp>

#include "msg-bus/events.h"
#include "activemq/msg-ifce.h"
#include "db/generic/TransferState.h"


namespace fts3 {
namespace server {

/**
 * SingleTrStateInstance class declaration
 **/
class SingleTrStateInstance
{
public:
    ~SingleTrStateInstance();

    /**
     * SingleTrStateInstance thread-safe upon init singleton
     **/
    static SingleTrStateInstance & instance()
    {
        if (i.get() == 0)
            {
                boost::mutex::scoped_lock lock(_mutex);
                if (i.get() == 0)
                    {
                        i.reset(new SingleTrStateInstance);
                    }
            }
        return *i;
    }

    void sendStateMessage(const std::string& jobId, uint64_t fileId);

private:
    SingleTrStateInstance(); // Private so that it can  not be called

    SingleTrStateInstance(SingleTrStateInstance const&) = delete;
    SingleTrStateInstance & operator=(SingleTrStateInstance const&) = delete;

    static boost::mutex _mutex;
    static std::unique_ptr<SingleTrStateInstance> i;

    std::string ftsAlias;

    bool monitoringMessages;
    boost::thread_specific_ptr<Producer> producer;
};

} // end namespace server
} // end namespace fts3

