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

#ifndef FETCHSTAGING_H_
#define FETCHSTAGING_H_

#include "common/ThreadPool.h"
#include "../task/Gfal2Task.h"
#include "../context/StagingContext.h"

#include "cred/DelegCred.h"

#include <boost/tuple/tuple.hpp>

#include <tuple>
#include <vector>
#include <string>
#include <map>

using namespace fts3::common;

/**
 * Fetches the staging jobs from DB in a separate thread
 */
class FetchStaging
{

public:
    FetchStaging(ThreadPool<Gfal2Task> & threadpool) : threadpool(threadpool) {}
    virtual ~FetchStaging() {}

    void fetch();

private:

    void recoverStartedTasks();

    StagingContext::context_type get_context(boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string, std::string> const & t)
    {
        return StagingContext::context_type(
                   boost::get<0>(t),
                   boost::get<1>(t),
                   boost::get<2>(t),
                   boost::get<3>(t),
                   boost::get<4>(t),
                   boost::get<5>(t),
                   boost::get<6>(t),
                   boost::get<7>(t),
                   boost::get<8>(t)
               );
    }

    // typedefs for convenience
    // vo, dn ,se, source_space_token
    typedef std::tuple<std::string, std::string, std::string, std::string> key_type;

    ThreadPool<Gfal2Task> & threadpool;
};

#endif /* FETCHSTAGING_H_ */
