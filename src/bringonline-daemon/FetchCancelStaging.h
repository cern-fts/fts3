/*
 * FetchCancelStaging.h
 *
 *  Created on: 3 Jul 2014
 *      Author: simonm
 */

#ifndef FETCHCANCELSTAGING_H_
#define FETCHCANCELSTAGING_H_

#include "common/ThreadPool.h"
#include "Gfal2Task.h"

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
