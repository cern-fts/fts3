/*
 * JobStatusGetter.cpp
 *
 *  Created on: 6 Aug 2014
 *      Author: simonm
 */

#include "JobStatusGetter.h"

namespace fts3 {
namespace ws {

template <>
tns3__FileTransferStatus* JobStatusGetter::make_status<tns3__FileTransferStatus>()
{
    return soap_new_tns3__FileTransferStatus(ctx, -1);
}

template <>
tns3__FileTransferStatus2* JobStatusGetter::make_status<tns3__FileTransferStatus2>()
{
    return soap_new_tns3__FileTransferStatus2(ctx, -1);
}

template void JobStatusGetter::get<tns3__FileTransferStatus>(std::vector<tns3__FileTransferStatus*> & ret);
template void JobStatusGetter::get<tns3__FileTransferStatus2>(std::vector<tns3__FileTransferStatus2*> & ret);

template <typename STATUS>
void JobStatusGetter::get(std::vector<STATUS*> & ret)
{
    std::vector<FileTransferStatus*>::iterator it;

    db.getTransferFileStatus(job, archive, offset, limit, statuses);

    for (it = statuses.begin(); it != statuses.end(); ++it)
        {
            FileTransferStatus* tmp = *it;

            STATUS* status = make_status<STATUS>();

            status->destSURL = soap_new_std__string(ctx, -1);
            *status->destSURL = tmp->destSURL;

            status->logicalName = soap_new_std__string(ctx, -1);
            *status->logicalName = tmp->logicalName;

            status->reason = soap_new_std__string(ctx, -1);
            *status->reason = tmp->reason;

            status->reason_USCOREclass = soap_new_std__string(ctx, -1);
            *status->reason_USCOREclass = tmp->reason_class;

            status->sourceSURL = soap_new_std__string(ctx, -1);
            *status->sourceSURL = tmp->sourceSURL;

            status->transferFileState = soap_new_std__string(ctx, -1);
            *status->transferFileState = tmp->transferFileState;

            if(tmp->transferFileState == "NOT_USED")
                {
                    status->duration = 0;
                    status->numFailures = 0;
                }
            else
                {
                    status->duration = tmp->finish_time - tmp->start_time;
                    status->numFailures = tmp->numFailures;
                }

            // Retries only on request!
            if (retry)
                {
                    db.getTransferRetries(tmp->fileId, retries);

                    std::vector<FileRetry*>::iterator ri;
                    for (ri = retries.begin(); ri != retries.end(); ++ri)
                        {
                            tns3__FileTransferRetry* retry = soap_new_tns3__FileTransferRetry(ctx, -1);
                            retry->attempt  = (*ri)->attempt;
                            retry->datetime = (*ri)->datetime;
                            retry->reason   = (*ri)->reason;
                            status->retries.push_back(retry);

                            delete *ri;
                        }
                    retries.clear();
                }

            ret.push_back(status);

            delete *it;
        }
    statuses.clear();
}

JobStatusGetter::~JobStatusGetter()
{
    release_vector(statuses);
    release_vector(retries);
}


} /* namespace ws */
} /* namespace fts3 */
