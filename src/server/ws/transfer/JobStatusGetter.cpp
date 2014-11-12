/*
 * JobStatusGetter.cpp
 *
 *  Created on: 6 Aug 2014
 *      Author: simonm
 */

#include "JobStatusGetter.h"

#include "common/JobStatusHandler.h"
#include "common/error.h"

namespace fts3
{
namespace ws
{

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

template <>
tns3__TransferJobSummary* JobStatusGetter::make_summary<tns3__TransferJobSummary>()
{
    return soap_new_tns3__TransferJobSummary(ctx, -1);
}

template <>
tns3__TransferJobSummary2* JobStatusGetter::make_summary<tns3__TransferJobSummary2>()
{
    return soap_new_tns3__TransferJobSummary2(ctx, -1);
}

template void JobStatusGetter::file_status<tns3__FileTransferStatus>(std::vector<tns3__FileTransferStatus*> &, bool);
template void JobStatusGetter::file_status<tns3__FileTransferStatus2>(std::vector<tns3__FileTransferStatus2*> &, bool);

template <typename STATUS>
void JobStatusGetter::file_status(std::vector<STATUS*> & ret, bool glite)
{
    std::vector<FileTransferStatus*>::iterator it;

    bool dm_job = db.isDmJob(job);

    if (dm_job)
        db.getDmFileStatus(job, archive, offset, limit, file_statuses);
    else
        db.getTransferFileStatus(job, archive, offset, limit, file_statuses);

    for (it = file_statuses.begin(); it != file_statuses.end(); ++it)
        {
            FileTransferStatus* tmp = *it;
            tmp->transferFileState = to_glite_state(tmp->transferFileState, glite);

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
                    if (tmp->finish_time > 0 && tmp->start_time > 0)
                        status->duration = tmp->finish_time - tmp->start_time;
                    else if(tmp->finish_time <= 0 && tmp->start_time > 0)
                        status->duration = time(NULL) - tmp->start_time;
                    else
                        status->duration = 0;
                    status->numFailures = tmp->numFailures;
                }

            status->staging = (int64_t*)soap_malloc(ctx, sizeof(int64_t));
            if (tmp->staging_finished > 0 && tmp->staging_start > 0)
                *status->staging = tmp->staging_finished - tmp->staging_start;
            else if (tmp->staging_finished <= 0 && tmp->staging_start > 0)
                *status->staging = time(NULL) - tmp->staging_start;
            else
                *status->staging = 0;

            // Retries only on request! This type of information exists only for transfer jobs
            if (retry && !dm_job)
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
                        }
                }
            ret.push_back(status);
        }
}

template void JobStatusGetter::job_summary<tns3__TransferJobSummary>(tns3__TransferJobSummary * & ret, bool glite);
template void JobStatusGetter::job_summary<tns3__TransferJobSummary2>(tns3__TransferJobSummary2 * & ret, bool glite);

template <typename SUMMARY>
void JobStatusGetter::job_summary(SUMMARY * & ret, bool glite)
{
    if (db.isDmJob(job))
        db.getDmJobStatus(job, archive, job_statuses);
    else
        db.getTransferJobStatus(job, archive, job_statuses);

    if(!job_statuses.empty())
        {
            ret = make_summary<SUMMARY>();
            ret->jobStatus = to_gsoap_status(**job_statuses.begin(), glite);

            JobStatusHandler& handler = JobStatusHandler::getInstance();
            ret->numActive = handler.countInState(JobStatusHandler::FTS3_STATUS_ACTIVE, job_statuses);
            ret->numCanceled = handler.countInState(JobStatusHandler::FTS3_STATUS_CANCELED, job_statuses);
            ret->numSubmitted = handler.countInState(JobStatusHandler::FTS3_STATUS_SUBMITTED, job_statuses);
            ret->numFinished = handler.countInState(JobStatusHandler::FTS3_STATUS_FINISHED, job_statuses);
            count_ready(ret, handler.countInState(JobStatusHandler::FTS3_STATUS_READY, job_statuses));
            ret->numFailed = handler.countInState(JobStatusHandler::FTS3_STATUS_FAILED, job_statuses);
            if (glite)
                {
                    ret->numSubmitted += handler.countInState(JobStatusHandler::FTS3_STATUS_STAGING, job_statuses);
                    ret->numSubmitted += handler.countInState(JobStatusHandler::FTS3_STATUS_DELETE, job_statuses);
                    ret->numActive += handler.countInState(JobStatusHandler::FTS3_STATUS_STARTED, job_statuses);
                }
            else
                {
                    ret->numStaging = handler.countInState(JobStatusHandler::FTS3_STATUS_STAGING, job_statuses);
                    ret->numStarted = handler.countInState(JobStatusHandler::FTS3_STATUS_STARTED, job_statuses);
                    ret->numDelete = handler.countInState(JobStatusHandler::FTS3_STATUS_DELETE, job_statuses);
                }
        }
    else
        {
            if (!glite) throw Err_Custom("requestID <" + job + "> was not found");
            ret = make_summary<SUMMARY>();
            ret->jobStatus = handleStatusExceptionForGLite();
        }
}

std::string JobStatusGetter::to_glite_state(std::string const & state, bool glite)
{
    // if it is not for glite
    if (!glite) return state;
    // check if it is fts3 only state
    if (state == "STARTED") return "ACTIVE";
    if (state == "STAGING") return "SUBMITTED";
    if (state == "DELETE") return "SUBMITTED";
    // if not there's nothing to do
    return state;
}

tns3__JobStatus * JobStatusGetter::to_gsoap_status(JobStatus const & job_status, bool glite)
{
    tns3__JobStatus * status = soap_new_tns3__JobStatus(ctx, -1);

    status->clientDN = soap_new_std__string(ctx, -1);
    *status->clientDN = job_status.clientDN;

    status->jobID = soap_new_std__string(ctx, -1);
    *status->jobID = job_status.jobID;

    status->jobStatus = soap_new_std__string(ctx, -1);
    *status->jobStatus = to_glite_state(job_status.jobStatus, glite);

    status->reason = soap_new_std__string(ctx, -1);
    *status->reason = job_status.reason;

    status->voName = soap_new_std__string(ctx, -1);
    *status->voName = job_status.voName;

    // change sec precision to msec precision so it is compatible with old glite cli
    status->submitTime = job_status.submitTime * 1000;
    status->numFiles = job_status.numFiles;
    status->priority = job_status.priority;

    return status;
}

void JobStatusGetter::job_status(tns3__JobStatus * & status, bool glite)
{
    if (db.isDmJob(job))
        db.getDmJobStatus(job, archive, job_statuses);
    else
        db.getTransferJobStatus(job, archive, job_statuses);

    if(!job_statuses.empty())
        {
            status = to_gsoap_status(**job_statuses.begin(), glite);
        }
    else
        {
            // throw Err_Custom("requestID <" + requestID + "> was not found");
            status = handleStatusExceptionForGLite();
        }
}

tns3__JobStatus* JobStatusGetter::handleStatusExceptionForGLite()
{
    // the value to be returned
    tns3__JobStatus* status;

    // For now since the glite clients are not compatible with exception from our gsoap version
    // we do not rise an exception, instead we send the error string as the job status
    // and at the begining of it we attache backspaces in order to erase 'Unknown transfer state '

    // the string that should be erased
    string replace = "Unknown transfer state ";

    // backspace
    const char bs = 8;
    // error message
    string msg = "getTransferJobStatus: RequestID <" + job + "> was not found";

    // add backspaces at the begining of the error message
    for (int i = 0; i < replace.size(); i++)
        msg = bs + msg;

    // create the status object
    status = soap_new_tns3__JobStatus(ctx, -1);

    // create the string for the error message
    status->jobStatus = soap_new_std__string(ctx, -1);
    *status->jobStatus = msg;

    // set all the rest to NULLs
    status->clientDN = 0;
    status->jobID = 0;
    status->numFiles = 0;
    status->priority = 0;
    status->reason = 0;
    status->voName = 0;
    status->submitTime = 0;

    return status;
}

JobStatusGetter::~JobStatusGetter()
{
    release_vector(file_statuses);
    release_vector(retries);
    release_vector(job_statuses);
}


} /* namespace ws */
} /* namespace fts3 */
