/*
 * FetchDeletion.h
 *
 *  Created on: 15 Jul 2014
 *      Author: roiser
 */

#ifndef FETCHDELETION_H_
#define FETCHDELETION_H_

#include "common/ThreadPool.h"
#include "Gfal2Task.h"

#include "cred/DelegCred.h"

#include <boost/scoped_ptr.hpp>

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

    //static bool isSrmUrl(const std::string & url);

    static std::string generateProxy(const std::string& dn, const std::string& dlg_id);

    ThreadPool<Gfal2Task> & threadpool;

};


inline static std::string generateProxy(const std::string& dn, const std::string& dlg_id)
{
  boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
  return delegCredPtr->getFileName(dn, dlg_id);
}

#endif /* FETCHDELETION_H_ */
