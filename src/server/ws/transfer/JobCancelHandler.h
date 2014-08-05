/*
 * JobCancelHandler.h
 *
 *  Created on: 4 Aug 2014
 *      Author: simonm
 */

#ifndef JOBCANCELHANDLER_H_
#define JOBCANCELHANDLER_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include "db/generic/SingleDbInstance.h"

#include <vector>
#include <string>

namespace fts3
{
namespace ws
{

class JobCancelHandler
{

public:

    JobCancelHandler(soap *ctx, std::vector<std::string> & jobs) :
        ctx(ctx), jobs(jobs), db(*db::DBSingleton::instance().getDBObjectInstance()) {}

    virtual ~JobCancelHandler() {}

    void cancel();

    void cancel(impltns__cancel2Response & resp);

private:

    std::string get_state(std::string const & job, std::string const & dn);
    void send_msg(std::string const & job);

    soap *ctx;
    std::vector<std::string> & jobs;
    GenericDbIfce& db;

    static std::string const CANCELED;
    static std::string const DOES_NOT_EXIST;
};

}
}

#endif /* JOBCANCELHANDLER_H_ */
