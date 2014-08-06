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

namespace fts3 {
namespace ws {

class JobStatusGetter
{

public:
    JobStatusGetter(soap * ctx, std::string const & job, bool archive, int offset, int limit, bool retry) :
        ctx(ctx), db(*db::DBSingleton::instance().getDBObjectInstance()),
        job(job), archive(archive), offset(offset), limit(limit), retry(retry) {}

    virtual ~JobStatusGetter();

    template <typename STATUS>
    void get(std::vector<STATUS*> & statuses);


private:

    template <typename STATUS>
    STATUS* make_status();

    template <typename T>
    void release_vector(std::vector<T*> & vec)
    {
        typename std::vector<T*>::iterator it;
        for (it = vec.begin(); it != vec.end(); ++it) delete *it;
    }

    soap * ctx;
    GenericDbIfce & db;

    std::string const & job;
    bool archive;
    int offset;
    int limit;
    bool retry;

    // keep vectors of pointers as fields so the can be released RAII style
    std::vector<FileTransferStatus*> statuses;
    std::vector<FileRetry*> retries;
};



} /* namespace ws */
} /* namespace fts3 */

#endif /* JOBSTATUSGETTER_H_ */
