/*
 * Copyright (c) CERN 2013-2019
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
#ifndef FETCHCANCELARCHIVING_H_
#define FETCHCANCELARCHIVING_H_

#include "common/ThreadPool.h"
#include "../task/Gfal2Task.h"


class FetchCancelArchiving
{
public:
    FetchCancelArchiving(fts3::common::ThreadPool<Gfal2Task> &threadpool) : threadpool(threadpool) {}
    virtual ~FetchCancelArchiving() {}

    void fetch();

private:
    fts3::common::ThreadPool<Gfal2Task> &threadpool;
};

#endif // FETCHCANCELARCHIVING_H_
