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
#include "db/generic/StorageElement.h"
#include "db/generic/SeGroup.h"
#include "db/generic/SeProtocolConfig.h"
#include "db/generic/ShareConfig.h"
#include "db/generic/OAuth.h"
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
        job.agentDn       = v.get<std::string>("agent_dn", "");
        job.submitHost    = v.get<std::string>("submit_host");
        job.userDn        = v.get<std::string>("user_dn");
        job.userCred      = v.get<std::string>("user_cred", "");
        job.credId        = v.get<std::string>("cred_id", "");
        job.spaceToken    = v.get<std::string>("space_token", "");
        job.storageClass  = v.get<std::string>("storage_class", "");
        job.internalJobParams = v.get<std::string>("job_params", "");
        job.overwriteFlag = v.get<std::string>("overwrite_flag", "N");
        job.sourceSpaceToken = v.get<std::string>("source_space_token", "");
        job.sourceSpaceTokenDescription = v.get<std::string>("source_token_description", "");
        job.copyPinLifetime = v.get<int>("copy_pin_lifetime", -1);
        job.bringOnline     = v.get<int>("bring_online", -1);
        job.checksumMethod  = v.get<std::string>("checksum_method", "");
        aux_tm = v.get<struct tm>("submit_time");
        job.submitTime = timegm(&aux_tm);

        try
            {
                job.reuse = v.get<std::string>("reuse_job");
            }
        catch (...)
            {
            }

        // No method that uses this type asks for finish_time
        job.finishTime = 0;
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
        file.fileId     = v.get<int>("file_id");
        file.overwriteFlag   = v.get<std::string>("overwrite_flag","");
        file.userDn          = v.get<std::string>("user_dn");
        file.credId     = v.get<std::string>("cred_id");
        file.checksum    = v.get<std::string>("checksum","");
        file.checksumMethod    = v.get<std::string>("checksum_method","");
        file.sourceSpaceToken = v.get<std::string>("source_space_token","");
        file.destinationSpaceToken   = v.get<std::string>("space_token","");
        file.bringOnline   = v.get<int>("bring_online",0);
        file.pinLifetime  = v.get<int>("copy_pin_lifetime",0);
        file.fileMetadata = v.get<std::string>("file_metadata", "");
        file.jobMetadata  = v.get<std::string>("job_metadata", "");
        file.userFilesize = v.get<double>("user_filesize", 0);
        file.fileIndex    = v.get<int>("file_index", 0);
        file.bringOnlineToken = v.get<std::string>("bringonline_token", "");
        file.sourceSe = v.get<std::string>("source_se", "");
        file.destSe = v.get<std::string>("dest_se", "");
        file.selectionStrategy = v.get<std::string>("selection_strategy", "");
        file.internalFileParams = v.get<std::string>("internal_job_params", "");
        file.userCredentials = v.get<std::string>("user_cred", "");
        try
            {
                file.reuseJob = v.get<std::string>("reuse_job", "");
            }
        catch(...)
            {
                // optional
            }

        // filesize and reason are NOT queried by any method that uses this
        // type
        file.filesize = 0;
    }
};

template <>
struct type_conversion<SeProtocolConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, SeProtocolConfig& protoConfig)
    {
        protoConfig.tcpBufferSize = v.get<int>("tcp_buffer_size", 0);
        protoConfig.numberOfStreams = v.get<int>("nostreams", 0);
        //protoConfig.unused = v.get<int>("no_tx_activity_to", 0);
        protoConfig.transferTimeout = v.get<int>("URLCOPY_TX_TO", 0);
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
        transfer.fileId            = v.get<int>("file_id");
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
struct type_conversion<StorageElement>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, StorageElement& se)
    {
        se.endpoint = v.get<std::string>("endpoint");
        se.seType  = v.get<std::string>("se_type");
        se.site     = v.get<std::string>("site");
        se.name     = v.get<std::string>("name");
        se.state    = v.get<std::string>("state");
        se.version  = v.get<std::string>("version");
        se.host     = v.get<std::string>("host");
        se.seTransferType     = v.get<std::string>("se_transfer_type");
        se.seTransferProtocol = v.get<std::string>("se_transfer_protocol");
        se.seControlProtocol  = v.get<std::string>("se_control_protocol");
        se.gocdb_id = v.get<std::string>("gocdb_id");
    }
};

template <>
struct type_conversion<SeConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, SeConfig& config)
    {
        config.source         = v.get<std::string>("source");
        config.destination    = v.get<std::string>("dest");
        config.vo             = v.get<std::string>("vo");
        config.symbolicName   = v.get<std::string>("symbolicName");
        config.state          = v.get<std::string>("state");
    }

};

template <>
struct type_conversion<ShareConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, ShareConfig& config)
    {
        config.source           = v.get<std::string>("source");
        config.destination      = v.get<std::string>("destination");
        config.vo               = v.get<std::string>("vo");
        config.activeTransfers = v.get<int>("active");
    }
};

template <>
struct type_conversion<SeGroup>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, SeGroup& grp)
    {
        grp.active       = v.get<int>("active", -1);
        grp.groupName    = v.get<std::string>("groupName");
        grp.member       = v.get<std::string>("member");
        grp.symbolicName = v.get<std::string>("symbolicName");
    }
};

template <>
struct type_conversion<LinkConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, LinkConfig& lnk)
    {
        lnk.source            = v.get<std::string>("source");
        lnk.destination       = v.get<std::string>("destination");
        lnk.state             = v.get<std::string>("state");
        lnk.symbolicName     = v.get<std::string>("symbolicName");
        lnk.numberOfStreams         = v.get<int>("nostreams");
        lnk.tcpBufferSize   = v.get<int>("tcp_buffer_size");
        lnk.transferTimeout     = v.get<int>("urlcopy_tx_to");
        //lnk.NO_TX_ACTIVITY_TO = v.get<int>("no_tx_activity_to");
        lnk.autoTuning     = v.get<std::string>("auto_tuning");
    }
};

template <>
struct type_conversion<FileRetry>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, FileRetry& retry)
    {
        retry.fileId   = v.get<int>("file_id");
        retry.attempt  = v.get<int>("attempt");
        retry.reason   = v.get<std::string>("reason");

        struct tm aux_tm;
        aux_tm = v.get<tm>("datetime");
        retry.datetime = timegm(&aux_tm);
    }
};

template<>
struct type_conversion<OAuth>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, OAuth& oauth)
    {
        oauth.appKey      = v.get<std::string>("app_key", "");
        oauth.appSecret   = v.get<std::string>("app_secret", "");
        oauth.accessToken = v.get<std::string>("access_token", "");
        oauth.accessTokenSecret = v.get<std::string>("access_token_secret", "");
    }
};

}
