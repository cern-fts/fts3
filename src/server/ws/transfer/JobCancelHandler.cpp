/*
 * JobCancelHandler.cpp
 *
 *  Created on: 4 Aug 2014
 *      Author: simonm
 */

#include "JobCancelHandler.h"

#include "ws/CGsiAdapter.h"
#include "ws/AuthorizationManager.h"
#include "ws/SingleTrStateInstance.h"

#include "common/logger.h"
#include "common/error.h"
#include "JobStatusHandler.h"

#include <algorithm>

#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>

namespace fts3
{
namespace ws
{

std::string const JobCancelHandler::CANCELED = "CANCELED";
std::string const JobCancelHandler::DOES_NOT_EXIST = "DOES_NOT_EXIST";

void JobCancelHandler::cancel()
{
    // jobs that will be cancelled
    std::vector<std::string> cancel, cancel_dm;
    // get the user DN
    CGsiAdapter cgsi (ctx);
    std::string const dn = cgsi.getClientDn();
    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << "is cancelling transfer jobs: ";
    // iterate over all jobs and check if they are suitable for cancelling
    std::vector<std::string>::const_iterator it;
    for (it = jobs.begin(); it != jobs.end(); ++it)
        {
            std::string const & job = *it;
            std::string status = get_state(job, dn);
            if (status == DOES_NOT_EXIST)
                throw Err_Custom("Transfer job: " + job + " does not exist!");
            else if (status != CANCELED)
                throw Err_Custom("Transfer job: " + job + " cannot be cancelled (it is in " + status + " state)");
            // if the job is OK add it to respective vector
            if (db.isDmJob(job))
                cancel_dm.push_back(job);
            else
                cancel.push_back(job);
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << job << ", ";
        }
    FTS3_COMMON_LOGGER_NEWLOG (INFO) << commit;
    // cancel the jobs
    if (!cancel.empty())
        {
            db.cancelJob(cancel);
            // and send a state changed message to monitoring
            std::for_each(cancel.begin(), cancel.end(), boost::bind(&JobCancelHandler::send_msg, this, _1));
        }
    if (!cancel_dm.empty())
        {
            db.cancelDmJobs(cancel_dm);
            // and send a state changed message to monitoring
            std::for_each(cancel_dm.begin(), cancel_dm.end(), boost::bind(&JobCancelHandler::send_msg, this, _1));
        }
}

void JobCancelHandler::cancel(impltns__cancel2Response & resp)
{
    // create the response
    resp._jobIDs = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(ctx, -1);
    resp._status = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(ctx, -1);
    // use references for convenience
    std::vector<std::string> & resp_jobs = resp._jobIDs->item;
    std::vector<std::string> & resp_stat = resp._status->item;
    // jobs that will be cancelled
    std::vector<std::string> cancel, cancel_dm;
    // get the user DN
    CGsiAdapter cgsi (ctx);
    std::string const dn = cgsi.getClientDn();
    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << "is cancelling transfer jobs: ";
    // iterate over all jobs and check if they are suitable for cancelling
    std::vector<std::string>::const_iterator it;
    for (it = jobs.begin(); it != jobs.end(); ++it)
        {
            std::string const & job = *it;
            std::string status = get_state(job, dn);
            resp_jobs.push_back(job);
            resp_stat.push_back(status);
            if (status == CANCELED)
                {
                    if (db.isDmJob(job))
                        cancel_dm.push_back(job);
                    else
                        cancel.push_back(job);
                    FTS3_COMMON_LOGGER_NEWLOG (INFO) << job << ", ";
                }
        }
    // in case no job was suitable for cancelling
    if (cancel.empty() && cancel_dm.empty())
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << " not a single job was suitable for cancelling ";
            return;
        }
    FTS3_COMMON_LOGGER_NEWLOG (INFO) << commit;
    // cancel the jobs
    if (!cancel.empty())
        {
            db.cancelJob(cancel);
            // and send a state changed message to monitoring
            std::for_each(cancel.begin(), cancel.end(), boost::bind(&JobCancelHandler::send_msg, this, _1));
        }
    if (!cancel_dm.empty())
        {
            db.cancelDmJobs(cancel_dm);
            // and send a state changed message to monitoring
            std::for_each(cancel_dm.begin(), cancel_dm.end(), boost::bind(&JobCancelHandler::send_msg, this, _1));
        }

}

std::string JobCancelHandler::get_state(std::string const & job, std::string const & dn)
{
    // get the transfer job object from DB
    boost::scoped_ptr<TransferJobs> job_ptr (db.getTransferJob(job, false));
    // if not throw an exception
    if (!job_ptr.get()) return DOES_NOT_EXIST;
    // Authorise the operation
    AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job_ptr.get());
    // get the status
    std::string const & status = job_ptr->JOB_STATE;
    // make sure the transfer-job is not in terminal state
    if (JobStatusHandler::getInstance().isTransferFinished(status))
        {
            return status;
        }

    return CANCELED;
}

void JobCancelHandler::send_msg(std::string const & job)
{
    std::vector<int> files;
    db.getFilesForJobInCancelState(job, files);

    std::vector<int>::const_iterator it;
    for (it = files.begin(); it != files.end(); ++it)
        {
            SingleTrStateInstance::instance().sendStateMessage(job, *it);
        }
}

}
}
