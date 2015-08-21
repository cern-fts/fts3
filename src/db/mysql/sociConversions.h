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

#include <Cred.h>
#include <CredCache.h>
#include <FileTransferStatus.h>
#include <JobStatus.h>
#include <LinkConfig.h>
#include <Se.h>
#include <SeAndConfig.h>
#include <SeGroup.h>
#include <SeProtocolConfig.h>
#include <ShareConfig.h>
#include <TransferJobs.h>
#include <OAuth.h>
#include <soci.h>
#include <time.h>

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
struct type_conversion<Cred>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, Cred& cred)
    {
        struct tm termination_st;
        cred.DN               = v.get<std::string>("dn");
        cred.delegationID     = v.get<std::string>("dlg_id");
        cred.proxy            = v.get<std::string>("proxy");
        termination_st        = v.get<struct tm>("termination_time");
        cred.termination_time = timegm(&termination_st);
        cred.vomsAttributes   = v.get<std::string>("voms_attrs", std::string());
    }
};

template <>
struct type_conversion<CredCache>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, CredCache& ccache)
    {
        ccache.DN                 = v.get<std::string>("dn");
        ccache.certificateRequest = v.get<std::string>("cert_request");
        ccache.delegationID       = v.get<std::string>("dlg_id");
        ccache.privateKey         = v.get<std::string>("priv_key");
        ccache.vomsAttributes     = v.get<std::string>("voms_attrs", std::string());
    }
};

template <>
struct type_conversion<TransferJobs>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, TransferJobs& job)
    {
        struct tm aux_tm;

        job.JOB_ID         = v.get<std::string>("job_id");
        job.JOB_STATE      = v.get<std::string>("job_state");
        job.VO_NAME        = v.get<std::string>("vo_name");
        job.PRIORITY       = v.get<int>("priority");
        job.SUBMIT_HOST    = v.get<std::string>("submit_host", "");
        job.SOURCE         = v.get<std::string>("source_se", "");
        job.DEST           = v.get<std::string>("dest_se", "");
        job.AGENT_DN       = v.get<std::string>("agent_dn", "");
        job.SUBMIT_HOST    = v.get<std::string>("submit_host");
        job.USER_DN        = v.get<std::string>("user_dn");
        job.USER_CRED      = v.get<std::string>("user_cred", "");
        job.CRED_ID        = v.get<std::string>("cred_id");
        job.SPACE_TOKEN    = v.get<std::string>("space_token", "");
        job.STORAGE_CLASS  = v.get<std::string>("storage_class", "");
        job.INTERNAL_JOB_PARAMS = v.get<std::string>("job_params");
        job.OVERWRITE_FLAG = v.get<std::string>("overwrite_flag");
        job.SOURCE_SPACE_TOKEN = v.get<std::string>("source_space_token", "");
        job.SOURCE_TOKEN_DESCRIPTION = v.get<std::string>("source_token_description", "");
        job.COPY_PIN_LIFETIME  = v.get<int>("copy_pin_lifetime");
        job.BRINGONLINE 	= v.get<int>("bring_online");
        job.CHECKSUM_METHOD    = v.get<std::string>("checksum_method");
        aux_tm = v.get<struct tm>("submit_time");
        job.SUBMIT_TIME = timegm(&aux_tm);

        try
            {
                job.REUSE = v.get<std::string>("reuse_job");
            }
        catch (...)
            {
            }

        // No method that uses this type asks for finish_time
        job.FINISH_TIME = 0;
    }
};

template <>
struct type_conversion<TransferFiles>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, TransferFiles& file)
    {
        file.FILE_STATE  = v.get<std::string>("file_state");
        file.SOURCE_SURL = v.get<std::string>("source_surl");
        file.DEST_SURL   = v.get<std::string>("dest_surl");
        file.JOB_ID      = v.get<std::string>("job_id");
        file.VO_NAME     = v.get<std::string>("vo_name");
        file.FILE_ID     = v.get<int>("file_id");
        file.OVERWRITE   = v.get<std::string>("overwrite_flag","");
        file.DN          = v.get<std::string>("user_dn");
        file.CRED_ID     = v.get<std::string>("cred_id");
        file.CHECKSUM    = v.get<std::string>("checksum","");
        file.CHECKSUM_METHOD    = v.get<std::string>("checksum_method","");
        file.SOURCE_SPACE_TOKEN = v.get<std::string>("source_space_token","");
        file.DEST_SPACE_TOKEN   = v.get<std::string>("space_token","");
        file.BRINGONLINE   = v.get<int>("bring_online",0);
        file.PIN_LIFETIME  = v.get<int>("copy_pin_lifetime",0);
        file.FILE_METADATA = v.get<std::string>("file_metadata", "");
        file.JOB_METADATA  = v.get<std::string>("job_metadata", "");
        file.USER_FILESIZE = v.get<double>("user_filesize", 0);
        file.FILE_INDEX    = v.get<int>("file_index", 0);
        file.BRINGONLINE_TOKEN = v.get<std::string>("bringonline_token", "");
        file.SOURCE_SE = v.get<std::string>("source_se", "");
        file.DEST_SE = v.get<std::string>("dest_se", "");
        file.SELECTION_STRATEGY = v.get<std::string>("selection_strategy", "");
        file.INTERNAL_FILE_PARAMS = v.get<std::string>("internal_job_params", "");
        file.USER_CREDENTIALS = v.get<std::string>("user_cred", "");
        try
            {
                file.REUSE_JOB = v.get<std::string>("reuse_job", "");
            }
        catch(...)
            {
                // optional
            }

        // filesize and reason are NOT queried by any method that uses this
        // type
        file.FILESIZE = "";
    }
};

template <>
struct type_conversion<SeAndConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, SeAndConfig& sc)
    {
        sc.SE_NAME     = v.get<std::string>("se_name");
        sc.SHARE_ID    = v.get<std::string>("share_id");
        sc.SHARE_TYPE  = v.get<std::string>("share_type");
        sc.SHARE_VALUE = v.get<std::string>("share_value");
    }
};

template <>
struct type_conversion<SeProtocolConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, SeProtocolConfig& protoConfig)
    {
        protoConfig.TCP_BUFFER_SIZE = v.get<int>("tcp_buffer_size", 0);
        protoConfig.NOSTREAMS = v.get<int>("nostreams", 0);
        protoConfig.NO_TX_ACTIVITY_TO = v.get<int>("no_tx_activity_to", 0);
        protoConfig.URLCOPY_TX_TO = v.get<int>("URLCOPY_TX_TO", 0);
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
        transfer.sourceSURL        = v.get<std::string>("source_surl");
        transfer.destSURL          = v.get<std::string>("dest_surl", "");
        transfer.transferFileState = v.get<std::string>("file_state");
        transfer.reason            = v.get<std::string>("reason", "");
        transfer.numFailures	   = v.get<int>("retry", 0);
        transfer.duration	       = v.get<double>("tx_duration", 0);

        if (v.get_indicator("start_time") == soci::i_ok)
            {
                aux_tm = v.get<struct tm>("start_time");
                transfer.start_time = timegm(&aux_tm);
            }
        else
            {
                transfer.start_time = 0;
            }
        if (v.get_indicator("finish_time") == soci::i_ok)
            {
                aux_tm = v.get<struct tm>("finish_time");
                transfer.finish_time = timegm(&aux_tm);
            }
        else
            {
                transfer.finish_time = 0;
            }

        try
            {
                if (v.get_indicator("staging_start") == soci::i_ok)
                    {
                        aux_tm = v.get<struct tm>("staging_start");
                        transfer.staging_start = timegm(&aux_tm);
                    }
                else
                    {
                        transfer.staging_start = 0;
                    }
                if (v.get_indicator("staging_finished") == soci::i_ok)
                    {
                        aux_tm = v.get<struct tm>("staging_finished");
                        transfer.staging_finished = timegm(&aux_tm);
                    }
                else
                    {
                        transfer.staging_finished = 0;
                    }
            }
        catch (...)
            {
                transfer.staging_start = transfer.staging_finished = 0;
            }
    }
};

template <>
struct type_conversion<Se>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, Se& se)
    {
        se.ENDPOINT = v.get<std::string>("endpoint");
        se.SE_TYPE  = v.get<std::string>("se_type");
        se.SITE     = v.get<std::string>("site");
        se.NAME     = v.get<std::string>("name");
        se.STATE    = v.get<std::string>("state");
        se.VERSION  = v.get<std::string>("version");
        se.HOST     = v.get<std::string>("host");
        se.SE_TRANSFER_TYPE     = v.get<std::string>("se_transfer_type");
        se.SE_TRANSFER_PROTOCOL = v.get<std::string>("se_transfer_protocol");
        se.SE_CONTROL_PROTOCOL  = v.get<std::string>("se_control_protocol");
        se.GOCDB_ID = v.get<std::string>("gocdb_id");
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
        config.active_transfers = v.get<int>("active");
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
        lnk.symbolic_name     = v.get<std::string>("symbolicName");
        lnk.NOSTREAMS         = v.get<int>("nostreams");
        lnk.TCP_BUFFER_SIZE   = v.get<int>("tcp_buffer_size");
        lnk.URLCOPY_TX_TO     = v.get<int>("urlcopy_tx_to");
        lnk.NO_TX_ACTIVITY_TO = v.get<int>("no_tx_activity_to");
        lnk.auto_tuning     = v.get<std::string>("auto_tuning");
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
        oauth.app_key      = v.get<std::string>("app_key", "");
        oauth.app_secret   = v.get<std::string>("app_secret", "");
        oauth.access_token = v.get<std::string>("access_token", "");
        oauth.access_token_secret = v.get<std::string>("access_token_secret", "");
    }
};

}
