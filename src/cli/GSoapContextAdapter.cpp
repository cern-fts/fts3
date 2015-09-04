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

#include "GSoapContextAdapter.h"
#include "MsgPrinter.h"

#include "delegation/ProxyCertificateDelegator.h"
#include "delegation/SoapDelegator.h"

#include "exception/cli_exception.h"
#include "exception/gsoap_error.h"


#include "fts3.nsmap"

#include "rest/ResponseParser.h"

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <cgsi_plugin.h>
#include <signal.h>

#include <sstream>
#include <fstream>

#include <algorithm>
#include <iterator>

#include <boost/lambda/lambda.hpp>

namespace fts3
{
namespace cli
{

std::vector<GSoapContextAdapter::Cleaner> GSoapContextAdapter::cleaners;

static std::string strFromStrPtr(const std::string* ptr)
{
    if (ptr)
        return *ptr;
    else
        return std::string();
}

GSoapContextAdapter::GSoapContextAdapter(const std::string& endpoint, const std::string& proxy):
    ServiceAdapter(endpoint), proxy(proxy), ctx(soap_new2(SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE))
{
    this->major = 0;
    this->minor = 0;
    this->patch = 0;

    ctx->socket_flags = MSG_NOSIGNAL;
    ctx->tcp_keep_alive = 1; // enable tcp keep alive
    ctx->bind_flags |= SO_REUSEADDR;
    ctx->max_keep_alive = 100; // at most 100 calls per keep-alive session
    ctx->recv_timeout = 120; // Timeout after 2 minutes stall on recv
    ctx->send_timeout = 120; // Timeout after 2 minute stall on send

    soap_set_imode(ctx, SOAP_ENC_MTOM | SOAP_IO_CHUNK);
    soap_set_omode(ctx, SOAP_ENC_MTOM | SOAP_IO_CHUNK);

    // Initialise CGSI plug-in
    if (endpoint.find("https") == 0)
        {
            if (soap_cgsi_init(ctx,  CGSI_OPT_DISABLE_NAME_CHECK | CGSI_OPT_SSL_COMPATIBLE)) throw gsoap_error(ctx);
        }
    else if (endpoint.find("httpg") == 0)
        {
            if (soap_cgsi_init(ctx, CGSI_OPT_DISABLE_NAME_CHECK )) throw gsoap_error(ctx);
        }

    if (!proxy.empty())
        cgsi_plugin_set_credentials(ctx, 0, proxy.c_str(), proxy.c_str());

    // set the namespaces
    if (soap_set_namespaces(ctx, fts3_namespaces)) throw gsoap_error(ctx);

    cleaners.push_back(Cleaner(this));
    signal(SIGINT, signalCallback);
    signal(SIGQUIT, signalCallback);
    signal(SIGILL, signalCallback);
    signal(SIGABRT, signalCallback);
    signal(SIGBUS, signalCallback);
    signal(SIGFPE, signalCallback);
    signal(SIGSEGV, signalCallback);
    signal(SIGPIPE, signalCallback);
    signal(SIGTERM, signalCallback);
    signal(SIGSTOP, signalCallback);
}

void GSoapContextAdapter::clean()
{
    soap_clr_omode(ctx, SOAP_IO_KEEPALIVE);
    shutdown(ctx->socket,2);
    shutdown(ctx->master,2);
    soap_destroy(ctx);
    soap_end(ctx);
    soap_done(ctx);
    soap_free(ctx);
}

GSoapContextAdapter::~GSoapContextAdapter()
{
    clean();
}

void GSoapContextAdapter::getInterfaceDetails()
{
    // request the information about the FTS3 service
    impltns__getInterfaceVersionResponse ivresp;
    int err = soap_call_impltns__getInterfaceVersion(ctx, endpoint.c_str(), 0, ivresp);
    if (!err)
        {
            interface = ivresp.getInterfaceVersionReturn;
            setInterfaceVersion(interface);
        }
    else
        {
            throw gsoap_error(ctx);
        }

    impltns__getVersionResponse vresp;
    err = soap_call_impltns__getVersion(ctx, endpoint.c_str(), 0, vresp);
    if (!err)
        {
            version = vresp.getVersionReturn;
        }
    else
        {
            throw gsoap_error(ctx);
        }

    impltns__getSchemaVersionResponse sresp;
    err = soap_call_impltns__getSchemaVersion(ctx, endpoint.c_str(), 0, sresp);
    if (!err)
        {
            schema = sresp.getSchemaVersionReturn;
        }
    else
        {
            throw gsoap_error(ctx);
        }

    impltns__getServiceMetadataResponse mresp;
    err = soap_call_impltns__getServiceMetadata(ctx, endpoint.c_str(), 0, "feature.string", mresp);
    if (!err)
        {
            metadata = mresp._getServiceMetadataReturn;
        }
    else
        {
            throw gsoap_error(ctx);
        }
}

std::string GSoapContextAdapter::transferSubmit(std::vector<File> const & files, std::map<std::string, std::string> const & parameters)
{
    // the transfer job
    tns3__TransferJob3 job;

    // the job's elements
    tns3__TransferJobElement3* element;

    // iterate over the internal vector containing job elements
    std::vector<File>::const_iterator f_it;
    for (f_it = files.begin(); f_it < files.end(); f_it++)
        {

            // create the job element, and set the source, destination and checksum values
            element = soap_new_tns3__TransferJobElement3(ctx, -1);

            // set the required fields (source and destination)
            element->source = f_it->sources;
            element->dest = f_it->destinations;

            // set the optional fields:


            element->checksum = f_it->checksums;

            // the file size
            if (f_it->file_size)
                {
                    element->filesize = (double*) soap_malloc(ctx, sizeof(double));
                    *element->filesize = *f_it->file_size;
                }
            else
                {
                    element->filesize = 0;
                }

            // the file metadata
            if (f_it->metadata)
                {
                    element->metadata = soap_new_std__string(ctx, -1);
                    *element->metadata = *f_it->metadata;
                }
            else
                {
                    element->metadata = 0;
                }

            if (f_it->selection_strategy)
                {
                    element->selectionStrategy = soap_new_std__string(ctx, -1);
                    *element->selectionStrategy = *f_it->selection_strategy;
                }
            else
                {
                    element->selectionStrategy = 0;
                }

            if (f_it->activity)
                {
                    element->activity = soap_new_std__string(ctx, -1);
                    *element->activity = *f_it->activity;
                }
            else
                {
                    element->activity = 0;
                }

            // push the element into the result vector
            job.transferJobElements.push_back(element);
        }


    job.jobParams = soap_new_tns3__TransferParams(ctx, -1);
    std::map<std::string, std::string>::const_iterator p_it;

    for (p_it = parameters.begin(); p_it != parameters.end(); p_it++)
        {
            job.jobParams->keys.push_back(p_it->first);
            job.jobParams->values.push_back(p_it->second);
        }

    impltns__transferSubmit4Response resp;
    if (soap_call_impltns__transferSubmit4(ctx, endpoint.c_str(), 0, &job, resp))
        throw gsoap_error(ctx);

    return resp._transferSubmit4Return;
}


void GSoapContextAdapter::delegate(std::string const & delegationId, long expirationTime)
{
    // delegate Proxy Certificate
    SoapDelegator delegator(endpoint, delegationId, expirationTime, proxy);
    delegator.delegate();
}


std::string GSoapContextAdapter::deleteFile (const std::vector<std::string>& filesForDelete)
{
    impltns__fileDeleteResponse resp;
    tns3__deleteFiles delFiles;

    std::vector<std::string>::const_iterator it;
    for (it = filesForDelete.begin(); it != filesForDelete.end(); ++it)
        delFiles.delf.push_back(*it);

    if (soap_call_impltns__fileDelete(ctx, endpoint.c_str(), 0, &delFiles,resp))
        throw gsoap_error(ctx);

    return resp._jobid;
}


JobStatus GSoapContextAdapter::getTransferJobStatus (std::string const & jobId, bool archive)
{
    tns3__JobRequest req;

    req.jobId   = jobId;
    req.archive = archive;

    impltns__getTransferJobStatus2Response resp;
    if (soap_call_impltns__getTransferJobStatus2(ctx, endpoint.c_str(), 0, &req, resp))
        throw gsoap_error(ctx);

    if (!resp.getTransferJobStatusReturn)
        throw cli_exception("The response from the server is empty!");

    long submitTime = resp.getTransferJobStatusReturn->submitTime / 1000;
    char time_buff[20];
    strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&submitTime));

    return JobStatus(
               strFromStrPtr(resp.getTransferJobStatusReturn->jobID),
               strFromStrPtr(resp.getTransferJobStatusReturn->jobStatus),
               strFromStrPtr(resp.getTransferJobStatusReturn->clientDN),
               strFromStrPtr(resp.getTransferJobStatusReturn->reason),
               strFromStrPtr(resp.getTransferJobStatusReturn->voName),
               time_buff,
               resp.getTransferJobStatusReturn->numFiles,
               resp.getTransferJobStatusReturn->priority
           );
}

std::vector< std::pair<std::string, std::string>  > GSoapContextAdapter::cancel(std::vector<std::string> const & jobIds)
{

    impltns__ArrayOf_USCOREsoapenc_USCOREstring rqst;
    rqst.item = jobIds;

    impltns__cancel2Response resp;


    if (soap_call_impltns__cancel2(ctx, endpoint.c_str(), 0, &rqst, resp))
        throw gsoap_error(ctx);

    std::vector< std::pair<std::string, std::string> > ret;

    if (resp._jobIDs && resp._status)
        {
            // zip two vectors
            std::vector<std::string> &ids = resp._jobIDs->item, &stats = resp._status->item;
            std::vector<std::string>::iterator itr_id = ids.begin(), itr_stat = stats.begin();

            for (; itr_id != ids.end() && itr_stat != stats.end(); ++itr_id, ++ itr_stat)
                {
                    ret.push_back(make_pair(*itr_id, *itr_stat));
                }
        }

    return ret;
}


boost::tuple<int, int> GSoapContextAdapter::cancelAll(const std::string& vo)
{
    impltns__cancelAllResponse resp;

    if (soap_call_impltns__cancelAll(ctx, endpoint.c_str(), 0, vo, resp))
        throw gsoap_error(ctx);

    return boost::tuple<int, int>(resp._jobCount, resp._fileCount);
}


void GSoapContextAdapter::getRoles (impltns__getRolesResponse& resp)
{
    if (soap_call_impltns__getRoles(ctx, endpoint.c_str(), 0, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::getRolesOf (std::string dn, impltns__getRolesOfResponse& resp)
{
    if (soap_call_impltns__getRolesOf(ctx, endpoint.c_str(), 0, dn, resp))
        throw gsoap_error(ctx);
}

std::vector<JobStatus> GSoapContextAdapter::listRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination)
{

    impltns__ArrayOf_USCOREsoapenc_USCOREstring* array = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(ctx, -1);
    array->item = statuses;

    impltns__listRequests2Response resp;
    if (soap_call_impltns__listRequests2(ctx, endpoint.c_str(), 0, array, "", dn, vo, source, destination, resp))
        throw gsoap_error(ctx);

    if (!resp._listRequests2Return)
        throw cli_exception("The response from the server is empty!");

    std::vector<JobStatus> ret;
    std::vector<tns3__JobStatus*>::iterator it;

    for (it = resp._listRequests2Return->item.begin(); it < resp._listRequests2Return->item.end(); it++)
        {
            tns3__JobStatus* gstat = *it;

            long submitTime = gstat->submitTime / 1000;
            char time_buff[20];
            strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&submitTime));

            JobStatus status (
                *gstat->jobID,
                *gstat->jobStatus,
                *gstat->clientDN,
                *gstat->reason,
                *gstat->voName,
                time_buff,
                gstat->numFiles,
                gstat->priority
            );
            ret.push_back(status);
        }

    return ret;
}

std::vector<JobStatus> GSoapContextAdapter::listDeletionRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination)
{
    impltns__ArrayOf_USCOREsoapenc_USCOREstring* array = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(ctx, -1);
    array->item = statuses;

    impltns__listDeletionRequestsResponse resp;
    if (soap_call_impltns__listDeletionRequests(ctx, endpoint.c_str(), 0, array, "", dn, vo, source, destination, resp))
        throw gsoap_error(ctx);

    if (!resp._listRequests2Return)
        throw cli_exception("The response from the server is empty!");

    std::vector<JobStatus> ret;
    std::vector<tns3__JobStatus*>::iterator it;

    for (it = resp._listRequests2Return->item.begin(); it < resp._listRequests2Return->item.end(); it++)
        {
            tns3__JobStatus* gstat = *it;

            long submitTime = gstat->submitTime / 1000;
            char time_buff[20];
            strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&submitTime));

            JobStatus status (
                *gstat->jobID,
                *gstat->jobStatus,
                *gstat->clientDN,
                *gstat->reason,
                *gstat->voName,
                time_buff,
                gstat->numFiles,
                gstat->priority
            );
            ret.push_back(status);
        }

    return ret;
}

void GSoapContextAdapter::listVoManagers(std::string vo, impltns__listVOManagersResponse& resp)
{
    if (soap_call_impltns__listVOManagers(ctx, endpoint.c_str(), 0, vo, resp))
        throw gsoap_error(ctx);
}

JobStatus GSoapContextAdapter::getTransferJobSummary (std::string const & jobId, bool archive)
{
    tns3__JobRequest req;

    req.jobId   = jobId;
    req.archive = archive;

    impltns__getTransferJobSummary3Response resp;
    if (soap_call_impltns__getTransferJobSummary3(ctx, endpoint.c_str(), 0, &req, resp))
        throw gsoap_error(ctx);

    if (!resp.getTransferJobSummary2Return)
        throw cli_exception("The response from the server is empty!");

    long submitTime = resp.getTransferJobSummary2Return->jobStatus->submitTime / 1000;
    char time_buff[20];
    strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&submitTime));

    JobStatus::JobSummary summary (
        resp.getTransferJobSummary2Return->numActive,
        resp.getTransferJobSummary2Return->numReady,
        resp.getTransferJobSummary2Return->numCanceled,
        resp.getTransferJobSummary2Return->numFinished,
        resp.getTransferJobSummary2Return->numSubmitted,
        resp.getTransferJobSummary2Return->numFailed,
        resp.getTransferJobSummary2Return->numStaging,
        resp.getTransferJobSummary2Return->numStarted,
        resp.getTransferJobSummary2Return->numDelete
    );

    return JobStatus(
               strFromStrPtr(resp.getTransferJobSummary2Return->jobStatus->jobID),
               strFromStrPtr(resp.getTransferJobSummary2Return->jobStatus->jobStatus),
               strFromStrPtr(resp.getTransferJobSummary2Return->jobStatus->clientDN),
               strFromStrPtr(resp.getTransferJobSummary2Return->jobStatus->reason),
               strFromStrPtr(resp.getTransferJobSummary2Return->jobStatus->voName),
               time_buff,
               resp.getTransferJobSummary2Return->jobStatus->numFiles,
               resp.getTransferJobSummary2Return->jobStatus->priority,
               summary
           );
}

std::vector<FileInfo> GSoapContextAdapter::getFileStatus (std::string const & jobId, bool archive, int offset, int limit, bool retries)
{
    tns3__FileRequest req;

    req.jobId   = jobId;
    req.archive = archive;
    req.offset  = offset;
    req.limit   = limit;
    req.retries = retries;

    impltns__getFileStatus3Response resp;
    if (soap_call_impltns__getFileStatus3(ctx, endpoint.c_str(), 0, &req, resp))
        throw gsoap_error(ctx);

    std::vector<FileInfo> ret;
    std::copy(resp.getFileStatusReturn->item.begin(), resp.getFileStatusReturn->item.end(), std::back_inserter(ret));
    return ret;
}

void GSoapContextAdapter::setConfiguration (config__Configuration *config, implcfg__setConfigurationResponse& resp)
{
    if (soap_call_implcfg__setConfiguration(ctx, endpoint.c_str(), 0, config, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::getConfiguration (std::string src, std::string dest, std::string all, std::string name, implcfg__getConfigurationResponse& resp)
{
    if (soap_call_implcfg__getConfiguration(ctx, endpoint.c_str(), 0, all, name, src, dest, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::delConfiguration(config__Configuration *config, implcfg__delConfigurationResponse& resp)
{
    if (soap_call_implcfg__delConfiguration(ctx, endpoint.c_str(), 0, config, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setMaxOpt(std::tuple<std::string, int, std::string> const & triplet, std::string const & opt)
{
    config__BringOnline bring_online;
    config__BringOnlineTriplet* t = soap_new_config__BringOnlineTriplet(ctx, -1);
    t->se = std::get<0>(triplet);
    t->value = std::get<1>(triplet);
    t->vo = std::get<2>(triplet);
    t->operation = opt;
    bring_online.boElem.push_back(t);
    implcfg__setBringOnlineResponse resp;
    if (soap_call_implcfg__setBringOnline(ctx, endpoint.c_str(), 0, &bring_online, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setBandwidthLimit(const std::string& source_se, const std::string& dest_se, int limit)
{
    config__BandwidthLimit bandwidth_limit;
    config__BandwidthLimitPair* pair = soap_new_config__BandwidthLimitPair(ctx, -1);
    pair->source = source_se;
    pair->dest   = dest_se;
    pair->limit  = limit;
    bandwidth_limit.blElem.push_back(pair);

    implcfg__setBandwidthLimitResponse resp;
    if (soap_call_implcfg__setBandwidthLimit(ctx, endpoint.c_str(), 0, &bandwidth_limit, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::getBandwidthLimit(implcfg__getBandwidthLimitResponse& resp)
{
    if (soap_call_implcfg__getBandwidthLimit(ctx, endpoint.c_str(), 0, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::debugSet(std::string source, std::string destination, unsigned level)
{
    impltns__debugLevelSetResponse resp;
    if (soap_call_impltns__debugLevelSet(ctx, endpoint.c_str(), 0, source, destination, level, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::blacklistDn(std::string subject, std::string status, int timeout, bool mode)
{

    impltns__blacklistDnResponse resp;
    if (soap_call_impltns__blacklistDn(ctx, endpoint.c_str(), 0, subject, mode, status, timeout, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::blacklistSe(std::string name, std::string vo, std::string status, int timeout, bool mode)
{

    impltns__blacklistSeResponse resp;
    if (soap_call_impltns__blacklistSe(ctx, endpoint.c_str(), 0, name, vo, status, timeout, mode, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::doDrain(bool drain)
{
    implcfg__doDrainResponse resp;
    if (soap_call_implcfg__doDrain(ctx, endpoint.c_str(), 0, drain, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::showUserDn(bool show)
{
    implcfg__showUserDnResponse resp;
    if (soap_call_implcfg__showUserDn(ctx, endpoint.c_str(), 0, show, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::prioritySet(std::string jobId, int priority)
{
    impltns__prioritySetResponse resp;
    if (soap_call_impltns__prioritySet(ctx, endpoint.c_str(), 0, jobId, priority, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setSeProtocol(std::string protocol, std::string se, std::string state)
{
    implcfg__setSeProtocolResponse resp;
    if (soap_call_implcfg__setSeProtocol(ctx, endpoint.c_str(), 0, protocol, se, state, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::retrySet(std::string vo, int retry)
{
    implcfg__setRetryResponse resp;
    if (soap_call_implcfg__setRetry(ctx, endpoint.c_str(), 0, vo, retry, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::optimizerModeSet(int mode)
{
    implcfg__setOptimizerModeResponse resp;
    if (soap_call_implcfg__setOptimizerMode(ctx, endpoint.c_str(), 0, mode, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::queueTimeoutSet(unsigned timeout)
{
    implcfg__setQueueTimeoutResponse resp;
    if (soap_call_implcfg__setQueueTimeout(ctx, endpoint.c_str(), 0, timeout, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setGlobalTimeout(int timeout)
{
    implcfg__setGlobalTimeoutResponse resp;
    if (soap_call_implcfg__setGlobalTimeout(ctx, endpoint.c_str(), 0, timeout, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setGlobalLimits(boost::optional<int> maxActivePerLink, boost::optional<int> maxActivePerSe)
{
    implcfg__setGlobalLimitsResponse resp;
    config__GlobalLimits req;

    if (maxActivePerLink.is_initialized())
        req.maxActivePerLink = &(*maxActivePerLink);
    else
        req.maxActivePerLink = NULL;
    if (maxActivePerSe.is_initialized())
        req.maxActivePerSe = &(*maxActivePerSe);
    else
        req.maxActivePerSe = NULL;

    if (soap_call_implcfg__setGlobalLimits(ctx, endpoint.c_str(), 0, &req, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::authorize(const std::string& op, const std::string& dn)
{
    implcfg__authorizeActionResponse resp;
    config__SetAuthz req;
    req.add = true;
    req.operation = op;
    req.dn = dn;
    if (soap_call_implcfg__authorizeAction(ctx, endpoint.c_str(), 0, &req, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::revoke(const std::string& op, const std::string& dn)
{
    implcfg__authorizeActionResponse resp;
    config__SetAuthz req;
    req.add = false;
    req.operation = op;
    req.dn = dn;
    if (soap_call_implcfg__authorizeAction(ctx, endpoint.c_str(), 0, &req, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setSecPerMb(int secPerMb)
{
    implcfg__setSecPerMbResponse resp;
    if (soap_call_implcfg__setSecPerMb(ctx, endpoint.c_str(), 0, secPerMb, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setMaxDstSeActive(std::string se, int active)
{
    implcfg__maxDstSeActiveResponse resp;
    if (soap_call_implcfg__maxDstSeActive(ctx, endpoint.c_str(), 0, se, active, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setMaxSrcSeActive(std::string se, int active)
{
    implcfg__maxSrcSeActiveResponse resp;
    if (soap_call_implcfg__maxSrcSeActive(ctx, endpoint.c_str(), 0, se, active, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setFixActivePerPair(std::string source, std::string destination, int active)
{
    implcfg__fixActivePerPairResponse resp;
    if (soap_call_implcfg__fixActivePerPair(ctx, endpoint.c_str(), 0, source, destination, active, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setS3Credential(std::string const & accessKey, std::string const & secretKey, std::string const & vo, std::string const & storage)
{
    implcfg__setS3CredentialResponse resp;
    if (soap_call_implcfg__setS3Credential(ctx, endpoint.c_str(), 0, accessKey, secretKey, vo, storage, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setDropboxCredential(std::string const & appKey, std::string const & appSecret, std::string const & apiUrl)
{
    implcfg__setDropboxCredentialResponse resp;
    if (soap_call_implcfg__setDropboxCredential(ctx, endpoint.c_str(), 0, appKey, appSecret, apiUrl, resp))
        throw gsoap_error(ctx);
}

std::vector<Snapshot> GSoapContextAdapter::getSnapShot(std::string const & vo, std::string const & src, std::string const & dst)
{
    impltns__getSnapshotResponse resp;
    if (soap_call_impltns__getSnapshot(ctx, endpoint.c_str(), 0, vo, src, dst, resp))
        throw gsoap_error(ctx);

    return ResponseParser(resp._result).getSnapshot(false);
}

std::vector<DetailedFileStatus> GSoapContextAdapter::getDetailedJobStatus(std::string const & jobId)
{
    impltns__detailedJobStatusResponse resp;
    if (soap_call_impltns__detailedJobStatus(ctx, endpoint.c_str(), 0, jobId, resp))
        throw gsoap_error(ctx);

    std::vector<DetailedFileStatus> ret;
    std::copy(resp._detailedJobStatus->transferStatus.begin(), resp._detailedJobStatus->transferStatus.end(), std::back_inserter(ret));

    return ret;
}

void GSoapContextAdapter::setInterfaceVersion(std::string interface)
{

    if (interface.empty()) return;

    // set the separator that will be used for tokenizing
    boost::char_separator<char> sep(".");
    boost::tokenizer< boost::char_separator<char> > tokens(interface, sep);
    boost::tokenizer< boost::char_separator<char> >::iterator it = tokens.begin();

    if (it == tokens.end()) return;

    std::string s = *it++;
    major = boost::lexical_cast<long>(s);

    if (it == tokens.end()) return;

    s = *it++;
    minor = boost::lexical_cast<long>(s);

    if (it == tokens.end()) return;

    s = *it;
    patch = boost::lexical_cast<long>(s);
}

void GSoapContextAdapter::signalCallback(int signum)
{
    // check if it is the right signal
    if (signum != SIGINT &&
            signum != SIGQUIT &&
            signum != SIGILL &&
            signum != SIGABRT &&
            signum != SIGBUS &&
            signum != SIGFPE &&
            signum != SIGSEGV &&
            signum != SIGPIPE &&
            signum != SIGTERM &&
            signum != SIGSTOP) exit(signum);
    // call all the cleaners
    for_each(cleaners.begin(), cleaners.end(), boost::lambda::_1);
    exit(signum);
}

}
}
