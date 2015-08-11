/*
 * FetchDeletion.h
 *
 *  Created on: 15 Jul 2014
 *      Author: roiser
 */

#ifndef FETCHDELETION_H_
#define FETCHDELETION_H_

#include "common/ThreadPool.h"
#include "task/Gfal2Task.h"

#include "cred/DelegCred.h"

#include <tuple>
#include <string>


/**
 * Fetches the deletion tasks from DB in a separate thread and puts
 */

class FetchDeletion
{

public:

    FetchDeletion(ThreadPool<Gfal2Task> & threadpool) : threadpool(threadpool) {}

    virtual ~FetchDeletion() {}

    void fetch();

private:

    // typedefs for convenience
    // vo, dn ,se
    typedef std::tuple<std::string, std::string, std::string> key_type;

    ThreadPool<Gfal2Task> & threadpool;

};

#endif /* FETCHDELETION_H_ */
