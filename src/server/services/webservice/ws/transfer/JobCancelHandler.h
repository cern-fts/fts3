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
