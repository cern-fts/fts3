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
 *
 * Conversion from and into SOCI
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
#include <soci.h>
#include <time.h>

namespace soci
{

inline time_t getTimeT(values const& v, const std::string& name)
{
    std::tm when;

    if (v.get_indicator(name) != soci::i_ok)
        return time(NULL);

    switch (v.get_properties(name).get_data_type())
        {
        case dt_double:
            return static_cast<time_t>(v.get<double>(name));
        case dt_integer:
            return static_cast<time_t>(v.get<int>(name));
        case dt_long_long:
            return static_cast<time_t>(v.get<long long>(name));
        case dt_unsigned_long_long:
            return static_cast<time_t>(v.get<unsigned long long>(name));
        case dt_date:
            when = v.get<std::tm>(name);
            return timegm(&when);
        case dt_string:
            strptime(v.get<std::string>(name).c_str(), "%d-%b-%y %H.%M.%S.000000 %p %z", &when);
            return timegm(&when);
        default:
            throw std::bad_cast();
        }
}


inline time_t getTimeT(row const& r, const std::string& name)
{
    std::tm when;

    if (r.get_indicator(name) != soci::i_ok)
        return time(NULL);

    switch (r.get_properties(name).get_data_type())
        {
        case dt_double:
            return static_cast<time_t>(r.get<double>(name));
        case dt_integer:
            return static_cast<time_t>(r.get<int>(name));
        case dt_long_long:
            return static_cast<time_t>(r.get<long long>(name));
        case dt_unsigned_long_long:
            return static_cast<time_t>(r.get<unsigned long long>(name));
        case dt_date:
            when = r.get<std::tm>(name);
            return timegm(&when);
        case dt_string:
            strptime(r.get<std::string>(name).c_str(), "%d-%b-%y %H.%M.%S.000000 %p %z", &when);
            return timegm(&when);
        default:
            throw std::bad_cast();
        }
}


template <>
struct type_conversion<Cred>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, Cred& cred)
    {
        cred.DN               = v.get<std::string>("DN");
        cred.delegationID     = v.get<std::string>("DLG_ID");
        cred.proxy            = v.get<std::string>("PROXY");
        cred.termination_time = getTimeT(v, "TERMINATION_TIME");
        cred.vomsAttributes   = v.get<std::string>("VOMS_ATTRS", std::string());
    }
};

template <>
struct type_conversion<CredCache>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, CredCache& ccache)
    {
        ccache.DN                 = v.get<std::string>("DN");
        ccache.certificateRequest = v.get<std::string>("CERT_REQUEST");
        ccache.delegationID       = v.get<std::string>("DLG_ID");
        ccache.privateKey         = v.get<std::string>("PRIV_KEY");
        ccache.vomsAttributes     = v.get<std::string>("VOMS_ATTRS", std::string());
    }
};

template <>
struct type_conversion<TransferJobs>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, TransferJobs& job)
    {
        job.JOB_ID         = v.get<std::string>("JOB_ID");
        job.JOB_STATE      = v.get<std::string>("JOB_STATE");
        job.VO_NAME        = v.get<std::string>("VO_NAME");
        job.PRIORITY       = static_cast<int>(v.get<long long>("PRIORITY"));
        job.SUBMIT_HOST    = v.get<std::string>("SUBMIT_HOST", "");
        job.SOURCE         = v.get<std::string>("SOURCE_SE", "");
        job.DEST           = v.get<std::string>("DEST_SE", "");
        job.AGENT_DN       = v.get<std::string>("AGENT_DN", "");
        job.SUBMIT_HOST    = v.get<std::string>("SUBMIT_HOST");
        job.USER_DN        = v.get<std::string>("USER_DN");
        job.USER_CRED      = v.get<std::string>("USER_CRED", "");
        job.CRED_ID        = v.get<std::string>("CRED_ID", "");
        job.SPACE_TOKEN    = v.get<std::string>("SPACE_TOKEN", "");
        job.STORAGE_CLASS  = v.get<std::string>("STORAGE_CLASS", "");
        job.INTERNAL_JOB_PARAMS = v.get<std::string>("JOB_PARAMS", "");
        job.OVERWRITE_FLAG = v.get<std::string>("OVERWRITE_FLAG", "");
        job.SOURCE_SPACE_TOKEN = v.get<std::string>("SOURCE_SPACE_TOKEN", "");
        job.SOURCE_TOKEN_DESCRIPTION = v.get<std::string>("SOURCE_TOKEN_DESCRIPTION", "");
        job.COPY_PIN_LIFETIME  = static_cast<int>(v.get<double>("COPY_PIN_LIFETIME"));
        job.BRINGONLINE 	= static_cast<int>(v.get<double>("BRING_ONLINE"));
        job.CHECKSUM_METHOD = v.get<std::string>("CHECKSUM_METHOD", "");
        job.SUBMIT_TIME     = getTimeT(v, "SUBMIT_TIME");

        try
            {
                job.REUSE = v.get<std::string>("REUSE_JOB", "");
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
        file.FILE_STATE  = v.get<std::string>("FILE_STATE");
        file.SOURCE_SURL = v.get<std::string>("SOURCE_SURL");
        file.DEST_SURL   = v.get<std::string>("DEST_SURL");
        file.JOB_ID      = v.get<std::string>("JOB_ID");
        file.VO_NAME     = v.get<std::string>("VO_NAME");
        file.FILE_ID     = static_cast<int>(v.get<long long>("FILE_ID"));
        file.OVERWRITE   = v.get<std::string>("OVERWRITE_FLAG", "");
        file.DN          = v.get<std::string>("USER_DN");
        file.CRED_ID     = v.get<std::string>("CRED_ID", "");
        file.CHECKSUM    = v.get<std::string>("CHECKSUM", "");
        file.CHECKSUM_METHOD    = v.get<std::string>("CHECKSUM_METHOD", "");
        file.SOURCE_SPACE_TOKEN = v.get<std::string>("SOURCE_SPACE_TOKEN", "");
        file.DEST_SPACE_TOKEN   = v.get<std::string>("SPACE_TOKEN", "");
        file.BRINGONLINE   = static_cast<int>(v.get<double>("BRING_ONLINE"),0);
        file.PIN_LIFETIME  = static_cast<int>(v.get<double>("COPY_PIN_LIFETIME"),0);
        file.FILE_METADATA = v.get<std::string>("FILE_METADATA", "");
        file.JOB_METADATA  = v.get<std::string>("JOB_METADATA", "");
        file.USER_FILESIZE = static_cast<double>(v.get<long long>("USER_FILESIZE", 0));
        file.FILE_INDEX    = static_cast<int>(v.get<long long>("FILE_INDEX", 0));
        file.BRINGONLINE_TOKEN = v.get<std::string>("BRINGONLINE_TOKEN", "");
        file.SOURCE_SE = v.get<std::string>("SOURCE_SE", "");
        file.DEST_SE = v.get<std::string>("DEST_SE", "");
        file.SELECTION_STRATEGY = v.get<std::string>("SELECTION_STRATEGY", "");
        file.INTERNAL_FILE_PARAMS = v.get<std::string>("INTERNAL_JOB_PARAMS", "");

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
        sc.SE_NAME     = v.get<std::string>("SE_NAME", "");
        sc.SHARE_ID    = v.get<std::string>("SHARE_ID", "");
        sc.SHARE_TYPE  = v.get<std::string>("SHARE_TYPE", "");
        sc.SHARE_VALUE = v.get<std::string>("SHARE_VALUE", "");
    }
};

template <>
struct type_conversion<SeProtocolConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, SeProtocolConfig& protoConfig)
    {
        protoConfig.TCP_BUFFER_SIZE = static_cast<int>(v.get<long long>("TCP_BUFFER_SIZE", 0));
        protoConfig.NOSTREAMS = static_cast<int>(v.get<double>("NOSTREAMS", 0));
        protoConfig.NO_TX_ACTIVITY_TO = static_cast<int>(v.get<long long>("NO_TX_ACTIVITY_TO", 0));
        protoConfig.URLCOPY_TX_TO = static_cast<int>(v.get<long long>("URLCOPY_TX_TO", 0));
    }
};

template <>
struct type_conversion<JobStatus>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, JobStatus& job)
    {
        job.jobID      = v.get<std::string>("JOB_ID");
        job.jobStatus  = v.get<std::string>("JOB_STATE");
        job.clientDN   = v.get<std::string>("USER_DN");
        job.reason     = v.get<std::string>("REASON", "");
        job.submitTime = getTimeT(v, "SUBMIT_TIME");
        job.priority   = static_cast<int>(v.get<long long>("PRIORITY"));
        job.voName     = v.get<std::string>("VO_NAME");

        try
            {
                // COUNT(*) type is long long inside soci
                job.numFiles   = v.get<long long>("NUMFILES");
            }
        catch (...)
            {
                // Ignore failures, since not all methods ask for this (i.e. getTransferJobStatus)
            }

        try
            {
                job.fileIndex  = static_cast<int>(v.get<long long>("FILE_INDEX"));
            }
        catch (...)
            {
                // Ignore failures, since not all methods ask for this (i.e. listRequests)
            }

        try
            {
                job.fileStatus = v.get<std::string>("FILE_STATE");
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
        transfer.fileId            = static_cast<int>(v.get<long long>("FILE_ID"));
        transfer.sourceSURL        = v.get<std::string>("SOURCE_SURL");
        transfer.destSURL          = v.get<std::string>("DEST_SURL");
        transfer.transferFileState = v.get<std::string>("FILE_STATE");
        transfer.reason            = v.get<std::string>("REASON", "");
        transfer.numFailures	   = static_cast<int>(v.get<double>("RETRY", 0));
        transfer.duration	   = v.get<double>("TX_DURATION",0);

        transfer.start_time        = getTimeT(v, "START_TIME");
        transfer.finish_time       = getTimeT(v, "FINISH_TIME");
    }
};

template <>
struct type_conversion<Se>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, Se& se)
    {
        se.ENDPOINT = v.get<std::string>("ENDPOINT", "");
        se.SE_TYPE  = v.get<std::string>("SE_TYPE", "");
        se.SITE     = v.get<std::string>("SITE", "");
        se.NAME     = v.get<std::string>("NAME", "");
        se.STATE    = v.get<std::string>("STATE", "");
        se.VERSION  = v.get<std::string>("VERSION", "");
        se.HOST     = v.get<std::string>("HOST", "");
        se.SE_TRANSFER_TYPE     = v.get<std::string>("SE_TRANSFER_TYPE", "");
        se.SE_TRANSFER_PROTOCOL = v.get<std::string>("SE_TRANSFER_PROTOCOL", "");
        se.SE_CONTROL_PROTOCOL  = v.get<std::string>("SE_CONTROL_PROTOCOL", "");
        se.GOCDB_ID = v.get<std::string>("GOCDB_ID", "");
    }
};

template <>
struct type_conversion<SeConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, SeConfig& config)
    {
        config.source         = v.get<std::string>("SOURCE", "");
        config.destination    = v.get<std::string>("DEST", "");
        config.vo             = v.get<std::string>("VO", "");
        config.symbolicName   = v.get<std::string>("SYMBOLICNAME", "");
        config.state          = v.get<std::string>("STATE", "");
    }

};

template <>
struct type_conversion<ShareConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, ShareConfig& config)
    {
        config.source           = v.get<std::string>("SOURCE", "");
        config.destination      = v.get<std::string>("DESTINATION", "");
        config.vo               = v.get<std::string>("VO", "");
        config.active_transfers = static_cast<int>(v.get<long long>("ACTIVE", -1));
    }
};

template <>
struct type_conversion<SeGroup>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, SeGroup& grp)
    {
        grp.active       = static_cast<int>(v.get<long long>("ACTIVE", -1));
        grp.groupName    = v.get<std::string>("GROUPNAME", "");
        grp.member       = v.get<std::string>("MEMBER", "");
        grp.symbolicName = v.get<std::string>("SYMBOLICNAME", "");
    }
};

template <>
struct type_conversion<LinkConfig>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, LinkConfig& lnk)
    {
        lnk.source            = v.get<std::string>("SOURCE", "");
        lnk.destination       = v.get<std::string>("DESTINATION", "");
        lnk.state             = v.get<std::string>("STATE", "");
        lnk.symbolic_name     = v.get<std::string>("SYMBOLICNAME", "");
        lnk.NOSTREAMS         = static_cast<int>(v.get<long long>("NOSTREAMS", 0));
        lnk.TCP_BUFFER_SIZE   = static_cast<int>(v.get<long long>("TCP_BUFFER_SIZE", 0));
        lnk.URLCOPY_TX_TO     = static_cast<int>(v.get<long long>("URLCOPY_TX_TO", 0));
        lnk.NO_TX_ACTIVITY_TO = static_cast<int>(v.get<long long>("NO_TX_ACTIVITY_TO", 0));
        lnk.auto_tuning     = v.get<std::string>("AUTO_TUNING", "");
    }
};

template <>
struct type_conversion<FileRetry>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, FileRetry& retry)
    {
        retry.fileId   = static_cast<int>(v.get<long long>("FILE_ID"));
        retry.attempt  = static_cast<int>(v.get<long long>("ATTEMPT", 0));
        retry.reason   = v.get<std::string>("REASON", "");

        retry.datetime = static_cast<int>(v.get<long long>("DATETIME"));
    }
};

}
