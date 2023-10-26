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

#pragma once

#include "db/generic/UserCredential.h"
#include "db/generic/UserCredentialCache.h"
#include "db/generic/FileTransferStatus.h"
#include "db/generic/JobStatus.h"
#include "db/generic/LinkConfig.h"
#include "db/generic/ShareConfig.h"
#include "db/generic/CloudStorageAuth.h"
#include "db/generic/TokenProvider.h"
#include <soci.h>
#include <time.h>
#include "../generic/Job.h"

inline time_t timeUTC()
{
    time_t now = time(NULL);
    struct tm *utc;
    utc = gmtime(&now);
    return timegm(utc);
}

namespace soci
{

template <>
struct type_conversion<Job::JobType>
{
    typedef std::string base_type;

    static void from_base(const std::string &c, indicator ind, Job::JobType &type)
    {
        if (ind == i_null || c.empty()) {
            type = Job::kTypeRegular;
        }
        else {
            type = static_cast<Job::JobType>(c[0]);
        }
    }

    static void to_base(const Job::JobType &type, std::string &c, indicator &ind)
    {
        c = std::string(1, static_cast<char>(type));
        ind = i_ok;
    }
};

template <>
struct type_conversion<UserCredential>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, UserCredential& cred)
    {
        struct tm termination_st;
        cred.userDn               = v.get<std::string>("dn");
        cred.delegationId     = v.get<std::string>("dlg_id");
        cred.proxy            = v.get<std::string>("proxy");
        termination_st        = v.get<struct tm>("termination_time");
        cred.terminationTime = timegm(&termination_st);
        cred.vomsAttributes   = v.get<std::string>("voms_attrs", std::string());
    }
};

template <>
struct type_conversion<UserCredentialCache>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, UserCredentialCache& ccache)
    {
        ccache.userDn                 = v.get<std::string>("dn");
        ccache.certificateRequest = v.get<std::string>("cert_request");
        ccache.delegationId       = v.get<std::string>("dlg_id");
        ccache.privateKey         = v.get<std::string>("priv_key");
        ccache.vomsAttributes     = v.get<std::string>("voms_attrs", std::string());
    }
};

template<>
struct type_conversion<Token>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, Token& token)
    {
        token.tokenId      = v.get<std::string>("token_id");
        token.accessToken  = v.get<std::string>("access_token", "");
        token.refreshToken = v.get<std::string>("refresh_token", "");
        token.issuer       = v.get<std::string>("issuer");

        auto issuer_size = token.issuer.size();
        if (token.issuer[issuer_size - 1] != '/') {
            token.issuer += "/";
        }
    }
};

template <>
struct type_conversion<Job>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, Job& job)
    {
        struct tm aux_tm;

        job.jobId         = v.get<std::string>("job_id");
        job.jobState      = v.get<std::string>("job_state");
        job.voName        = v.get<std::string>("vo_name");
        job.priority      = v.get<int>("priority");
        job.submitHost    = v.get<std::string>("submit_host", "");
        job.source        = v.get<std::string>("source_se", "");
        job.destination   = v.get<std::string>("dest_se", "");
        job.submitHost    = v.get<std::string>("submit_host");
        job.userDn        = v.get<std::string>("user_dn");
        job.credId        = v.get<std::string>("cred_id", "");
        job.spaceToken    = v.get<std::string>("space_token", "");
        job.storageClass  = v.get<std::string>("storage_class", "");
        job.internalJobParams = v.get<std::string>("job_params", "");
        job.overwriteFlag = v.get<std::string>("overwrite_flag", "N");
        job.sourceSpaceToken = v.get<std::string>("source_space_token", "");
        job.copyPinLifetime = v.get<int>("copy_pin_lifetime", -1);
        job.bringOnline     = v.get<int>("bring_online", -1);
        job.checksumMode  = v.get<std::string>("checksum_method", "");
        aux_tm = v.get<struct tm>("submit_time");
        job.submitTime = timegm(&aux_tm);

        job.jobType = v.get<Job::JobType>("job_type", Job::kTypeRegular);
    }
};

template <>
struct type_conversion<TransferFile>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, TransferFile& file)
    {
        file.fileState  = v.get<std::string>("file_state");
        file.sourceSurl = v.get<std::string>("source_surl");
        file.destSurl   = v.get<std::string>("dest_surl");
        file.jobId      = v.get<std::string>("job_id");
        file.voName     = v.get<std::string>("vo_name");
        file.fileId     = v.get<unsigned long long>("file_id");
        file.overwriteFlag   = v.get<std::string>("overwrite_flag","");
        file.dstFileReport   = v.get<std::string>("dst_file_report","");
        file.archiveTimeout  = v.get<int>("archive_timeout",-1);
        file.userDn          = v.get<std::string>("user_dn");
        file.credId     = v.get<std::string>("cred_id");
        file.checksum    = v.get<std::string>("checksum","");
        file.checksumMode    = v.get<std::string>("checksum_method","");
        file.sourceSpaceToken = v.get<std::string>("source_space_token","");
        file.destinationSpaceToken   = v.get<std::string>("space_token","");
        file.bringOnline   = v.get<int>("bring_online",0);
        file.pinLifetime  = v.get<int>("copy_pin_lifetime",0);
        file.fileMetadata = v.get<std::string>("file_metadata", "");
        file.transferMetadata = v.get<std::string>("archive_metadata", "");
        file.jobMetadata  = v.get<std::string>("job_metadata", "");
        file.userFilesize = v.get<long long>("user_filesize", 0);
        file.fileIndex    = v.get<int>("file_index", 0);
        file.bringOnlineToken = v.get<std::string>("bringonline_token", "");
        file.sourceSe = v.get<std::string>("source_se", "");
        file.destSe = v.get<std::string>("dest_se", "");
        file.selectionStrategy = v.get<std::string>("selection_strategy", "");
        file.internalFileParams = v.get<std::string>("internal_job_params", "");
        file.scitag = v.get<int>("scitag", 0);

        try {
            file.jobType = v.get<Job::JobType>("job_type", Job::kTypeRegular);
        }
        catch (...) {
            // optional
        }

        // filesize and reason are NOT queried by any method that uses this
        // type
        file.filesize = 0;
    }
};

template <>
struct type_conversion<JobStatus>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, JobStatus& job)
    {
        struct tm aux_tm;
        job.jobID      = v.get<std::string>("job_id");
        job.jobStatus  = v.get<std::string>("job_state");
        job.clientDN   = v.get<std::string>("user_dn");
        job.reason     = v.get<std::string>("reason", "");
        aux_tm         = v.get<struct tm>("submit_time");
        job.submitTime = timegm(&aux_tm);
        job.priority   = v.get<int>("priority");
        job.voName     = v.get<std::string>("vo_name");

        try
            {
                // COUNT(*) type is long long inside soci
                job.numFiles   = v.get<long long>("numFiles");
            }
        catch (...)
            {
                // Ignore failures, since not all methods ask for this (i.e. getTransferJobStatus)
            }

        try
            {
                job.fileIndex  = v.get<int>("file_index");
            }
        catch (...)
            {
                // Ignore failures, since not all methods ask for this (i.e. listRequests)
            }

        try
            {
                job.fileStatus = v.get<std::string>("file_state");
            }
        catch (...)
            {
                // Ignore failures, since not all methods ask for this (i.e. listRequests)
            }
    }
};

template <>
struct type_conversion<FileTransferStatus>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, FileTransferStatus& transfer)
    {
        struct tm aux_tm;
        transfer.fileId            = v.get<unsigned long long>("file_id");
        transfer.sourceSurl        = v.get<std::string>("source_surl");
        transfer.destSurl          = v.get<std::string>("dest_surl", "");
        transfer.fileState = v.get<std::string>("file_state");
        transfer.reason            = v.get<std::string>("reason", "");
        transfer.numFailures       = v.get<int>("retry", 0);
        transfer.duration          = v.get<double>("tx_duration", 0);

        if (v.get_indicator("start_time") == soci::i_ok)
            {
                aux_tm = v.get<struct tm>("start_time");
                transfer.startTime = timegm(&aux_tm);
            }
        else
            {
                transfer.startTime = 0;
            }
        if (v.get_indicator("finish_time") == soci::i_ok)
            {
                aux_tm = v.get<struct tm>("finish_time");
                transfer.finishTime = timegm(&aux_tm);
            }
        else
            {
                transfer.finishTime = 0;
            }

        try
            {
                if (v.get_indicator("staging_start") == soci::i_ok)
                    {
                        aux_tm = v.get<struct tm>("staging_start");
                        transfer.stagingStart = timegm(&aux_tm);
                    }
                else
                    {
                        transfer.stagingStart = 0;
                    }
                if (v.get_indicator("staging_finished") == soci::i_ok)
                    {
                        aux_tm = v.get<struct tm>("staging_finished");
                        transfer.stagingFinished = timegm(&aux_tm);
                    }
                else
                    {
                        transfer.stagingFinished = 0;
                    }
            }
        catch (...)
            {
                transfer.stagingStart = transfer.stagingFinished = 0;
            }
    }
};

template <>
struct type_conversion<ShareConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, ShareConfig& config)
    {
        config.source      = v.get<std::string>("source");
        config.destination = v.get<std::string>("destination");
        config.vo          = v.get<std::string>("vo");
        config.weight      = v.get<int>("active");
    }
};

template <>
struct type_conversion<LinkConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, LinkConfig& lnk)
    {
        lnk.source          = v.get<std::string>("source");
        lnk.destination     = v.get<std::string>("destination");
        lnk.minActive       = v.get<int>("min_active", 0);
        lnk.maxActive       = v.get<int>("max_active", 0);
        lnk.optimizerMode   = v.get<OptimizerMode>("optimizer_mode", kOptimizerDisabled);
        lnk.tcpBufferSize   = v.get<int>("tcp_buffer_size", 0);
        lnk.numberOfStreams = v.get<int>("nostreams", 1);
    }
};

template <>
struct type_conversion<FileRetry>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, FileRetry& retry)
    {
        retry.fileId   = v.get<unsigned long long>("file_id");
        retry.attempt  = v.get<int>("attempt");
        retry.reason   = v.get<std::string>("reason");

        struct tm aux_tm;
        aux_tm = v.get<tm>("datetime");
        retry.datetime = timegm(&aux_tm);
    }
};

template<>
struct type_conversion<CloudStorageAuth>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, CloudStorageAuth& auth)
    {
        auth.appKey      = v.get<std::string>("app_key", "");
        auth.appSecret   = v.get<std::string>("app_secret", "");
        auth.accessToken = v.get<std::string>("access_token", "");
        auth.accessTokenSecret = v.get<std::string>("access_token_secret", "");
        auth.requestToken = v.get<std::string>("request_token", "");
    }
};

template<>
struct type_conversion<boost::logic::tribool>
{
    typedef int base_type;

    static void from_base(int v, indicator ind, boost::logic::tribool& tribool)
    {
        if (ind == soci::i_null) {
            tribool = boost::logic::indeterminate;
        }
        else {
            tribool = (v != 0);
        }
    }
};

template<>
struct type_conversion<bool>
{
    typedef int base_type;

    static void from_base(int v, indicator ind, bool& boolean)
    {
        if (ind == soci::i_null) {
            boolean = false;
        }
        else {
            boolean = (v != 0);
        }
    }
};

template<>
struct type_conversion<OptimizerMode>
{
    typedef int base_type;

    static void from_base(int v, indicator ind, OptimizerMode& mode)
    {
        if (ind == soci::i_null) {
            mode = kOptimizerDisabled;
        }
        else if (v > kOptimizerAggressive) {
            mode = kOptimizerAggressive;
        }
        else if (v < 0) {
            mode = kOptimizerDisabled;
        }
        else {
            mode = static_cast<OptimizerMode>(v);
        }
    }
};

template<>
struct type_conversion<StorageConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, StorageConfig& seConfig)
    {
        seConfig.storage = v.get<std::string>("storage", "");
        seConfig.site = v.get<std::string>("site", "");
        seConfig.metadata = v.get<std::string>("metadata", "");
        seConfig.ipv6 = v.get<boost::tribool>("ipv6", boost::indeterminate);
        seConfig.udt = v.get<boost::tribool>("udt", boost::indeterminate);
        seConfig.debugLevel = v.get<int>("debug_level", 0);
        seConfig.inboundMaxActive = v.get<int>("inbound_max_active", 0);
        seConfig.outboundMaxActive = v.get<int>("outbound_max_active", 0);
        seConfig.inboundMaxThroughput = v.get<double>("inbound_max_throughput", 0.0);
        seConfig.outboundMaxThroughput = v.get<double>("outbound_max_throughput", 0.0);
    }
};

template<>
struct type_conversion<TokenProvider>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, TokenProvider& tokenProvider)
    {
        tokenProvider.name = v.get<std::string>("name");
        tokenProvider.issuer = v.get<std::string>("issuer");
        tokenProvider.clientId = v.get<std::string>("client_id");
        tokenProvider.clientSecret = v.get<std::string>("client_secret");

        auto issuer_size = tokenProvider.issuer.size();
        if (tokenProvider.issuer[issuer_size - 1] != '/') {
            tokenProvider.issuer += "/";
        }
    }
};

}
