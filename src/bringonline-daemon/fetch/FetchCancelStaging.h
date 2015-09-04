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

#ifndef FETCHCANCELSTAGING_H_
#define FETCHCANCELSTAGING_H_

#include "common/ThreadPool.h"
#include "../task/Gfal2Task.h"

#include <boost/tuple/tuple.hpp>

using namespace fts3::common;

class FetchCancelStaging
{

public:
    FetchCancelStaging(ThreadPool<Gfal2Task> & threadpool) : threadpool(threadpool) {}
    virtual ~FetchCancelStaging() {}

    void fetch();

private:

    ThreadPool<Gfal2Task> & threadpool;
};

#endif /* FETCHCANCELSTAGING_H_ */
