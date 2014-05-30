/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * GSoapContexAdapter.cpp
 *
 *  Created on: May 30, 2012
 *      Author: Michał Simon
 */

#include "GSoapContextAdapter.h"

#include "exception/cli_exception.h"
#include "exception/gsoap_error.h"


#include "ws-ifce/gsoap/fts3.nsmap"

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <cgsi_plugin.h>
#include <signal.h>

#include <sstream>
#include <fstream>

#include <algorithm>
#include <boost/lambda/lambda.hpp>

namespace fts3
{
namespace cli
{

vector<GSoapContextAdapter::Cleaner> GSoapContextAdapter::cleaners;

GSoapContextAdapter::GSoapContextAdapter(string endpoint):
    endpoint(endpoint), ctx(soap_new2(SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE)/*soap_new1(SOAP_ENC_MTOM)*/)
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

void GSoapContextAdapter::init()
{

    int  err = 0;

    // initialize cgsi plugin
    if (endpoint.find("https") == 0)
        {
            //err = soap_cgsi_init(ctx, CGSI_OPT_KEEP_ALIVE | CGSI_OPT_DISABLE_NAME_CHECK | CGSI_OPT_SSL_COMPATIBLE);
            err = soap_cgsi_init(ctx,  CGSI_OPT_DISABLE_NAME_CHECK | CGSI_OPT_SSL_COMPATIBLE);
        }
    else if (endpoint.find("httpg") == 0)
        {
            err = soap_cgsi_init(ctx, CGSI_OPT_DISABLE_NAME_CHECK );
        }

    if (err)
        {
            handleSoapFault("Failed to initialize the GSI plugin.");
        }

    // set the namespaces
    if (soap_set_namespaces(ctx, fts3_namespaces))
        {
            handleSoapFault("Failed to set SOAP namespaces.");
        }

    // request the information about the FTS3 service
    impltns__getInterfaceVersionResponse ivresp;
    err = soap_call_impltns__getInterfaceVersion(ctx, endpoint.c_str(), 0, ivresp);
    if (!err)
        {
            interface = ivresp.getInterfaceVersionReturn;
            setInterfaceVersion(interface);
        }
    else
        {
            handleSoapFault("Failed to determine the interface version of the service: getInterfaceVersion.");
        }

    impltns__getVersionResponse vresp;
    err = soap_call_impltns__getVersion(ctx, endpoint.c_str(), 0, vresp);
    if (!err)
        {
            version = vresp.getVersionReturn;
        }
    else
        {
            handleSoapFault("Failed to determine the version of the service: getVersion.");
        }

    impltns__getSchemaVersionResponse sresp;
    err = soap_call_impltns__getSchemaVersion(ctx, endpoint.c_str(), 0, sresp);
    if (!err)
        {
            schema = sresp.getSchemaVersionReturn;
        }
    else
        {
            handleSoapFault("Failed to determine the schema version of the service: getSchemaVersion.");
        }

    impltns__getServiceMetadataResponse mresp;
    err = soap_call_impltns__getServiceMetadata(ctx, endpoint.c_str(), 0, "feature.string", mresp);
    if (!err)
        {
            metadata = mresp._getServiceMetadataReturn;
        }
    else
        {
            handleSoapFault("Failed to determine the service metadata of the service: getServiceMetadataReturn.");
        }
}

string GSoapContextAdapter::transferSubmit (vector<File> const & files, map<string, string> const & parameters)
{
    // the transfer job
    tns3__TransferJob3 job;

    // the job's elements
    tns3__TransferJobElement3* element;

    // iterate over the internal vector containing job elements
    vector<File>::const_iterator f_it;
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

            // push the element into the result vector
            job.transferJobElements.push_back(element);
        }


    job.jobParams = soap_new_tns3__TransferParams(ctx, -1);
    map<string, string>::const_iterator p_it;

    for (p_it = parameters.begin(); p_it != parameters.end(); p_it++)
        {
            job.jobParams->keys.push_back(p_it->first);
            job.jobParams->values.push_back(p_it->second);
        }

    impltns__transferSubmit4Response resp;
    if (soap_call_impltns__transferSubmit4(ctx, endpoint.c_str(), 0, &job, resp))
        handleSoapFault("Failed to submit transfer: transferSubmit3.");

    return resp._transferSubmit4Return;
}

JobStatus GSoapContextAdapter::getTransferJobStatus (string jobId, bool archive)
{
    tns3__JobRequest req;

    req.jobId   = jobId;
    req.archive = archive;

    impltns__getTransferJobStatus2Response resp;
    if (soap_call_impltns__getTransferJobStatus2(ctx, endpoint.c_str(), 0, &req, resp))
        handleSoapFault("Failed to get job status: getTransferJobStatus.");

    if (!resp.getTransferJobStatusReturn)
        throw cli_exception("The response from the server is empty!");

    long submitTime = resp.getTransferJobStatusReturn->submitTime / 1000;
    char time_buff[20];
    strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&submitTime));

    return JobStatus(
               *resp.getTransferJobStatusReturn->jobID,
               *resp.getTransferJobStatusReturn->jobStatus,
               *resp.getTransferJobStatusReturn->clientDN,
               *resp.getTransferJobStatusReturn->reason,
               *resp.getTransferJobStatusReturn->voName,
               time_buff,
               resp.getTransferJobStatusReturn->numFiles,
               resp.getTransferJobStatusReturn->priority
           );
}

vector< pair<string, string> > GSoapContextAdapter::cancel(vector<string> jobIds)
{

    impltns__ArrayOf_USCOREsoapenc_USCOREstring rqst;
    rqst.item = jobIds;

    impltns__cancel2Response resp;


    if (soap_call_impltns__cancel2(ctx, endpoint.c_str(), 0, &rqst, resp))
        handleSoapFault("Failed to cancel jobs: cancel.");

    vector< pair<string, string> > ret;

    if (resp._jobIDs && resp._status)
        {
            // zip two vectors
            vector<string> &ids = resp._jobIDs->item, &stats = resp._status->item;
            vector<string>::iterator itr_id = ids.begin(), itr_stat = stats.begin();

            for (; itr_id != ids.end() && itr_stat != stats.end(); ++itr_id, ++ itr_stat)
                {
                    ret.push_back(make_pair(*itr_id, *itr_stat));
                }
        }

    return ret;
}

void GSoapContextAdapter::getRoles (impltns__getRolesResponse& resp)
{
    if (soap_call_impltns__getRoles(ctx, endpoint.c_str(), 0, resp))
        handleSoapFault("Failed to get roles: getRoles.");
}

void GSoapContextAdapter::getRolesOf (string dn, impltns__getRolesOfResponse& resp)
{
    if (soap_call_impltns__getRolesOf(ctx, endpoint.c_str(), 0, dn, resp))
        handleSoapFault("Failed to get roles: getRolesOf.");
}

vector<JobStatus> GSoapContextAdapter::listRequests (vector<string> statuses, string dn, string vo)
{

    impltns__ArrayOf_USCOREsoapenc_USCOREstring* array = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(ctx, -1);
    array->item = statuses;

    impltns__listRequests2Response resp;
    if (soap_call_impltns__listRequests2(ctx, endpoint.c_str(), 0, array, dn, vo, resp))
        handleSoapFault("Failed to list requests: listRequests2.");

    if (!resp._listRequests2Return)
        throw cli_exception("The response from the server is empty!");

    vector<JobStatus> ret;
    vector<tns3__JobStatus*>::iterator it;

    for (it = resp._listRequests2Return->item.begin(); it < resp._listRequests2Return->item.end(); it++)
        {
            tns3__JobStatus* gstat = *it;

            long submitTime = gstat->submitTime / 1000;
            char time_buff[20];
            strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&submitTime));

            JobStatus status = JobStatus(
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

void GSoapContextAdapter::listVoManagers(string vo, impltns__listVOManagersResponse& resp)
{
    if (soap_call_impltns__listVOManagers(ctx, endpoint.c_str(), 0, vo, resp))
        handleSoapFault("Failed to list VO managers: listVOManagers.");
}

JobSummary GSoapContextAdapter::getTransferJobSummary (string jobId, bool archive)
{
    tns3__JobRequest req;

    req.jobId   = jobId;
    req.archive = archive;

    impltns__getTransferJobSummary3Response resp;
    if (soap_call_impltns__getTransferJobSummary3(ctx, endpoint.c_str(), 0, &req, resp))
        handleSoapFault("Failed to get job status: getTransferJobSummary2.");

    if (!resp.getTransferJobSummary2Return)
        throw cli_exception("The response from the server is empty!");

    long submitTime = resp.getTransferJobSummary2Return->jobStatus->submitTime / 1000;
    char time_buff[20];
    strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&submitTime));

    JobStatus status = JobStatus(
                           *resp.getTransferJobSummary2Return->jobStatus->jobID,
                           *resp.getTransferJobSummary2Return->jobStatus->jobStatus,
                           *resp.getTransferJobSummary2Return->jobStatus->clientDN,
                           *resp.getTransferJobSummary2Return->jobStatus->reason,
                           *resp.getTransferJobSummary2Return->jobStatus->voName,
                           time_buff,
                           resp.getTransferJobSummary2Return->jobStatus->numFiles,
                           resp.getTransferJobSummary2Return->jobStatus->priority
                       );

    return JobSummary (
               status,
               resp.getTransferJobSummary2Return->numActive,
               resp.getTransferJobSummary2Return->numCanceled,
               resp.getTransferJobSummary2Return->numFailed,
               resp.getTransferJobSummary2Return->numFinished,
               resp.getTransferJobSummary2Return->numSubmitted,
               resp.getTransferJobSummary2Return->numReady
           );
}

int GSoapContextAdapter::getFileStatus (string jobId, bool archive, int offset, int limit,
                                        bool retries,
                                        impltns__getFileStatusResponse& resp)
{
    tns3__FileRequest req;

    req.jobId   = jobId;
    req.archive = archive;
    req.offset  = offset;
    req.limit   = limit;
    req.retries = retries;

    impltns__getFileStatus3Response resp3;
    if (soap_call_impltns__getFileStatus3(ctx, endpoint.c_str(), 0, &req, resp3))
        handleSoapFault("Failed to get job status: getFileStatus.");

    resp._getFileStatusReturn = resp3.getFileStatusReturn;

    return static_cast<int>(resp._getFileStatusReturn->item.size());
}

void GSoapContextAdapter::setConfiguration (config__Configuration *config, implcfg__setConfigurationResponse& resp)
{
    if (soap_call_implcfg__setConfiguration(ctx, endpoint.c_str(), 0, config, resp))
        handleSoapFault("Failed to set configuration: setConfiguration.");
}

void GSoapContextAdapter::getConfiguration (string src, string dest, string all, string name, implcfg__getConfigurationResponse& resp)
{
    if (soap_call_implcfg__getConfiguration(ctx, endpoint.c_str(), 0, all, name, src, dest, resp))
        handleSoapFault("Failed to get configuration: getConfiguration.");
}

void GSoapContextAdapter::delConfiguration(config__Configuration *config, implcfg__delConfigurationResponse& resp)
{
    if (soap_call_implcfg__delConfiguration(ctx, endpoint.c_str(), 0, config, resp))
        handleSoapFault("Failed to delete configuration: delConfiguration.");
}

void GSoapContextAdapter::setBringOnline(map<string, int>& pairs)
{

    config__BringOnline bring_online;

    map<string, int>::iterator it;
    for (it = pairs.begin(); it != pairs.end(); it++)
        {
            config__BringOnlinePair* pair = soap_new_config__BringOnlinePair(ctx, -1);
            pair->first = it->first;
            pair->second = it->second;
            bring_online.boElem.push_back(pair);
        }

    implcfg__setBringOnlineResponse resp;
    if (soap_call_implcfg__setBringOnline(ctx, endpoint.c_str(), 0, &bring_online, resp))
        handleSoapFault("Failed to set bring-online limit: setBringOnline.");
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
        handleSoapFault("Failed to set bandwidth limit: setBandwidthLimit.");
}

void GSoapContextAdapter::getBandwidthLimit(implcfg__getBandwidthLimitResponse& resp)
{
    if (soap_call_implcfg__getBandwidthLimit(ctx, endpoint.c_str(), 0, resp))
        handleSoapFault("Failed to set bandwidth limit: getBandwidthLimit.");
}

void GSoapContextAdapter::debugSet(string source, string destination, bool debug)
{
    impltns__debugSetResponse resp;
    if (soap_call_impltns__debugSet(ctx, endpoint.c_str(), 0, source, destination, debug, resp))
        {
            handleSoapFault("Operation debugSet failed.");
        }
}

void GSoapContextAdapter::blacklistDn(string subject, string status, int timeout, bool mode)
{

    impltns__blacklistDnResponse resp;
    if (soap_call_impltns__blacklistDn(ctx, endpoint.c_str(), 0, subject, mode, status, timeout, resp))
        {
            handleSoapFault("Operation blacklist failed.");
        }

}

void GSoapContextAdapter::blacklistSe(string name, string vo, string status, int timeout, bool mode)
{

    impltns__blacklistSeResponse resp;
    if (soap_call_impltns__blacklistSe(ctx, endpoint.c_str(), 0, name, vo, status, timeout, mode, resp))
        {
            handleSoapFault("Operation blacklist failed.");
        }
}

void GSoapContextAdapter::doDrain(bool drain)
{
    implcfg__doDrainResponse resp;
    if (soap_call_implcfg__doDrain(ctx, endpoint.c_str(), 0, drain, resp))
        {
            handleSoapFault("Operation doDrain failed.");
        }
}

void GSoapContextAdapter::prioritySet(string jobId, int priority)
{
    impltns__prioritySetResponse resp;
    if (soap_call_impltns__prioritySet(ctx, endpoint.c_str(), 0, jobId, priority, resp))
        {
            handleSoapFault("Operation prioritySet failed.");
        }
}

void GSoapContextAdapter::setSeProtocol(string protocol, string se, string state)
{
    implcfg__setSeProtocolResponse resp;
    if (soap_call_implcfg__setSeProtocol(ctx, endpoint.c_str(), 0, protocol, se, state, resp))
        {
            handleSoapFault("Operation setSeProtocol failed.");
        }
}

void GSoapContextAdapter::retrySet(int retry)
{
    implcfg__setRetryResponse resp;
    if (soap_call_implcfg__setRetry(ctx, endpoint.c_str(), 0, retry, resp))
        {
            handleSoapFault("Operation retrySet failed.");
        }
}

void GSoapContextAdapter::optimizerModeSet(int mode)
{
    implcfg__setOptimizerModeResponse resp;
    if (soap_call_implcfg__setOptimizerMode(ctx, endpoint.c_str(), 0, mode, resp))
        {
            handleSoapFault("Operation retrySet failed.");
        }
}

void GSoapContextAdapter::queueTimeoutSet(unsigned timeout)
{
    implcfg__setQueueTimeoutResponse resp;
    if (soap_call_implcfg__setQueueTimeout(ctx, endpoint.c_str(), 0, timeout, resp))
        {
            handleSoapFault("Operation queueTimeoutSet failed.");
        }
}

void GSoapContextAdapter::setGlobalTimeout(int timeout)
{
    implcfg__setGlobalTimeoutResponse resp;
    if (soap_call_implcfg__setGlobalTimeout(ctx, endpoint.c_str(), 0, timeout, resp))
        {
            handleSoapFault("Operation setGlobalTimeout failed.");
        }
}

void GSoapContextAdapter::setSecPerMb(int secPerMb)
{
    implcfg__setSecPerMbResponse resp;
    if (soap_call_implcfg__setSecPerMb(ctx, endpoint.c_str(), 0, secPerMb, resp))
        {
            handleSoapFault("Operation setSecPerMb failed.");
        }
}

void GSoapContextAdapter::setMaxDstSeActive(string se, int active)
{
    implcfg__maxDstSeActiveResponse resp;
    if (soap_call_implcfg__maxDstSeActive(ctx, endpoint.c_str(), 0, se, active, resp))
        {
            handleSoapFault("Operation maxDstSeActive failed.");
        }
}

void GSoapContextAdapter::setMaxSrcSeActive(string se, int active)
{
    implcfg__maxSrcSeActiveResponse resp;
    if (soap_call_implcfg__maxSrcSeActive(ctx, endpoint.c_str(), 0, se, active, resp))
        {
            handleSoapFault("Operation maxSrcSeActive failed.");
        }
}

std::string GSoapContextAdapter::getSnapShot(string vo, string src, string dst)
{
    impltns__getSnapshotResponse resp;
    if (soap_call_impltns__getSnapshot(ctx, endpoint.c_str(), 0, vo, src, dst, resp))
        {
            handleSoapFault("Operation failed.");
        }

    return resp._result;
}

void GSoapContextAdapter::handleSoapFault(string /*msg*/)
{
    throw gsoap_error(ctx);
}

void GSoapContextAdapter::setInterfaceVersion(string interface)
{

    if (interface.empty()) return;

    // set the seperator that will be used for tokenizing
    boost::char_separator<char> sep(".");
    boost::tokenizer< boost::char_separator<char> > tokens(interface, sep);
    boost::tokenizer< boost::char_separator<char> >::iterator it = tokens.begin();

    if (it == tokens.end()) return;

    string s = *it++;
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
