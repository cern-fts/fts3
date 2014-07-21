/*
 * FetchStaging.h
 *
 *  Created on: 2 Jul 2014
 *      Author: simonm
 */

#ifndef FETCHSTAGING_H_
#define FETCHSTAGING_H_

#include "common/ThreadPool.h"
#include "Gfal2Task.h"
#include "StagingContext.h"

#include "cred/DelegCred.h"


#include <boost/scoped_ptr.hpp>

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

    static bool isSrmUrl(const std::string & url)
    {
        if (url.compare(0, 6, "srm://") == 0)
            return true;

        return false;
    }

    static std::string generateProxy(const std::string& dn, const std::string& dlg_id)
    {
        boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
        return delegCredPtr->getFileName(dn, dlg_id);
    }

    ThreadPool<Gfal2Task> & threadpool;
};

#endif /* FETCHSTAGING_H_ */
