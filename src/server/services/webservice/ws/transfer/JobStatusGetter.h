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

#ifndef JOBSTATUSGETTER_H_
#define JOBSTATUSGETTER_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"
#include "db/generic/SingleDbInstance.h"

#include <vector>

namespace fts3
{
namespace ws
{

class JobStatusGetter
{

public:
    JobStatusGetter(soap * ctx, std::string const & jobId, bool archive, int offset, int limit, bool retry) :
        ctx(ctx), db(*db::DBSingleton::instance().getDBObjectInstance()),
        jobId(jobId), archive(archive), offset(offset), limit(limit), retry(retry) {}

    JobStatusGetter(soap * ctx, std::string const & jobId, bool archive) :
        ctx(ctx), db(*db::DBSingleton::instance().getDBObjectInstance()),
        jobId(jobId), archive(archive), offset(0), limit(0), retry(false) {}

    virtual ~JobStatusGetter();

    void job_status(tns3__JobStatus * & ret, bool glite = false);
    tns3__JobStatus* handleStatusExceptionForGLite();

    template <typename SUMMARY>
    void job_summary(SUMMARY * & ret, bool glite = false);

    template <typename STATUS>
    void file_status(std::vector<STATUS*> & statuses, bool glite = false);


private:

    template <typename STATUS>
    STATUS* make_status();

    template <typename SUMMARY>
    SUMMARY* make_summary();

    void count_ready(tns3__TransferJobSummary *, int)
    {
        // do nothing, there is no ready state in this structure
    }

    void count_ready(tns3__TransferJobSummary2 * summary, int count)
    {
        summary->numReady = count;
    }

    tns3__JobStatus * to_gsoap_status(Job const & job_status, int numFiles, bool glite);

    std::string to_glite_state(std::string const & state, bool glite);

    soap * ctx;
    GenericDbIfce & db;

    std::string const & jobId;
    bool archive;
    int offset;
    int limit;
    bool retry;
};



} /* namespace ws */
} /* namespace fts3 */

#endif /* JOBSTATUSGETTER_H_ */
