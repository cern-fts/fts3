/*
 * JobStatusGetter.h
 *
 *  Created on: 6 Aug 2014
 *      Author: simonm
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
    JobStatusGetter(soap * ctx, std::string const & job, bool archive, int offset, int limit, bool retry) :
        ctx(ctx), db(*db::DBSingleton::instance().getDBObjectInstance()),
        job(job), archive(archive), offset(offset), limit(limit), retry(retry) {}

    JobStatusGetter(soap * ctx, std::string const & job, bool archive) :
        ctx(ctx), db(*db::DBSingleton::instance().getDBObjectInstance()),
        job(job), archive(archive), offset(0), limit(0), retry(false) {}

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

    template <typename T>
    void release_vector(std::vector<T*> & vec)
    {
        typename std::vector<T*>::iterator it;
        for (it = vec.begin(); it != vec.end(); ++it) delete *it;
    }

    void count_ready(tns3__TransferJobSummary *, int)
    {
        // do nothing, there is no ready state in this structure
    }

    void count_ready(tns3__TransferJobSummary2 * summary, int count)
    {
        summary->numReady = count;
    }

    tns3__JobStatus * to_gsoap_status(JobStatus const & job_status, bool glite = false);

    std::string to_glite_state(std::string const & state, bool glite);

    soap * ctx;
    GenericDbIfce & db;

    std::string const & job;
    bool archive;
    int offset;
    int limit;
    bool retry;

    // keep vectors of pointers as fields so the can be released RAII style
    std::vector<FileTransferStatus*> file_statuses;
    std::vector<FileRetry*> retries;
    std::vector<JobStatus*> job_statuses;
};



} /* namespace ws */
} /* namespace fts3 */

#endif /* JOBSTATUSGETTER_H_ */
