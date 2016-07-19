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

class GSoapSupportRemoved: public cli_exception {
public:
    GSoapSupportRemoved(): cli_exception("gSOAP support removed for this operation") {
    }
};

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

    if (!proxy.empty() && access(proxy.c_str(), R_OK) == 0) {
        cgsi_plugin_set_credentials(ctx, 0, proxy.c_str(), proxy.c_str());
    }

    // set the namespaces
    if (soap_set_namespaces(ctx, fts3_namespaces)) throw gsoap_error(ctx);
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

std::string GSoapContextAdapter::transferSubmit(std::vector<File> const &, std::map<std::string, std::string> const &)
{
    throw GSoapSupportRemoved();
}


void GSoapContextAdapter::delegate(std::string const &, long)
{
    throw GSoapSupportRemoved();
}

long GSoapContextAdapter::isCertValid()
{
    throw GSoapSupportRemoved();
}

std::string GSoapContextAdapter::deleteFile(const std::vector<std::string> &)
{
    throw GSoapSupportRemoved();
}

JobStatus GSoapContextAdapter::getTransferJobStatus(std::string const &, bool)
{
    throw GSoapSupportRemoved();
}

std::vector<std::pair<std::string, std::string> > GSoapContextAdapter::cancel(std::vector<std::string> const &)
{
    throw GSoapSupportRemoved();
}

boost::tuple<int, int> GSoapContextAdapter::cancelAll(const std::string &)
{
    throw GSoapSupportRemoved();
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

std::vector<JobStatus> GSoapContextAdapter::listRequests(std::vector<std::string> const &, std::string const &,
    std::string const &, std::string const &, std::string const &)
{
    throw GSoapSupportRemoved();
}

std::vector<JobStatus> GSoapContextAdapter::listDeletionRequests(std::vector<std::string> const &, std::string const &,
    std::string const &, std::string const &, std::string const &)
{
    throw GSoapSupportRemoved();
}


JobStatus GSoapContextAdapter::getTransferJobSummary(std::string const &, bool)
{
    throw GSoapSupportRemoved();
}

std::vector<FileInfo> GSoapContextAdapter::getFileStatus(std::string const &, bool, int, int, bool)
{
    throw GSoapSupportRemoved();
}

void GSoapContextAdapter::setConfiguration (config__Configuration *config, implcfg__setConfigurationResponse& resp)
{
    if (soap_call_implcfg__setConfiguration(ctx, endpoint.c_str(), 0, config, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::setConfiguration (std::vector<std::string> const &cfgs)
{
    config__Configuration *config = soap_new_config__Configuration(ctx, -1);
    config->cfg = cfgs;
    implcfg__setConfigurationResponse resp;
    if (soap_call_implcfg__setConfiguration(ctx, endpoint.c_str(), 0, config, resp))
        throw gsoap_error(ctx);
    soap_delete(ctx, &resp);
    soap_delete(ctx, config);
}

void GSoapContextAdapter::getConfiguration (std::string src, std::string dest, std::string all, std::string name, implcfg__getConfigurationResponse& resp)
{
    if (soap_call_implcfg__getConfiguration(ctx, endpoint.c_str(), 0, all, name, src, dest, resp))
        throw gsoap_error(ctx);
}

std::vector<std::string> GSoapContextAdapter::getConfiguration (std::string src, std::string dest, std::string all, std::string name)
{
    implcfg__getConfigurationResponse resp;
    if (soap_call_implcfg__getConfiguration(ctx, endpoint.c_str(), 0, all, name, src, dest, resp))
        throw gsoap_error(ctx);
    std::vector<std::string> cfg = resp.configuration->cfg;
    soap_delete(ctx, &resp);
    return cfg;
}

void GSoapContextAdapter::delConfiguration(config__Configuration *config, implcfg__delConfigurationResponse& resp)
{
    if (soap_call_implcfg__delConfiguration(ctx, endpoint.c_str(), 0, config, resp))
        throw gsoap_error(ctx);
}

void GSoapContextAdapter::delConfiguration(std::vector<std::string> const &cfgs)
{
    config__Configuration *config = soap_new_config__Configuration(ctx, -1);
    config->cfg = cfgs;
    implcfg__delConfigurationResponse resp;
    if (soap_call_implcfg__delConfiguration(ctx, endpoint.c_str(), 0, config, resp))
        throw gsoap_error(ctx);
    soap_delete(ctx,&resp);
    soap_delete(ctx,config);
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

std::string GSoapContextAdapter::getBandwidthLimit()
{
    implcfg__getBandwidthLimitResponse resp;
    if (soap_call_implcfg__getBandwidthLimit(ctx, endpoint.c_str(), 0, resp))
        throw gsoap_error(ctx);
    std::string limit = resp.limit;
    soap_delete(ctx, &resp);
    return limit;
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

}
}
