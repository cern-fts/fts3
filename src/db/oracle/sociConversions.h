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
#include "db/generic/Job.h"
#include "db/generic/OAuth.h"
#include <soci.h>
#include <time.h>

namespace soci
{

inline time_t getTimeT(values const& v, const std::string& name)
{
    std::tm when;
    std::string str_repr;

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
            return mktime(&when);
        case dt_string:
            str_repr = v.get<std::string>(name);
            strptime(str_repr.c_str(), "%d-%b-%y %I.%M.%S.000000 %p %z", &when);
            return mktime(&when);
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
            return mktime(&when);
        case dt_string:
            strptime(r.get<std::string>(name).c_str(), "%d-%b-%y %I.%M.%S.000000 %p %z", &when);
            return mktime(&when);
        default:
            throw std::bad_cast();
        }
}


template <>
struct type_conversion<UserCredential>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, UserCredential& cred)
    {
        cred.userDn               = v.get<std::string>("DN");
        cred.delegationId     = v.get<std::string>("DLG_ID");
        cred.proxy            = v.get<std::string>("PROXY");
        cred.terminationTime = getTimeT(v, "TERMINATION_TIME");
        cred.vomsAttributes   = v.get<std::string>("VOMS_ATTRS", std::string());
    }
};

template <>
struct type_conversion<UserCredentialCache>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, UserCredentialCache& ccache)
    {
        ccache.userDn                 = v.get<std::string>("DN");
        ccache.certificateRequest = v.get<std::string>("CERT_REQUEST");
        ccache.delegationId       = v.get<std::string>("DLG_ID");
        ccache.privateKey         = v.get<std::string>("PRIV_KEY");
        ccache.vomsAttributes     = v.get<std::string>("VOMS_ATTRS", std::string());
    }
};

template <>
struct type_conversion<Job>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, Job& job)
    {
        job.jobId         = v.get<std::string>("JOB_ID");
        job.jobState      = v.get<std::string>("JOB_STATE");
        job.voName        = v.get<std::string>("VO_NAME");
        job.priority       = static_cast<int>(v.get<long long>("PRIORITY"));
        job.submitHost    = v.get<std::string>("SUBMIT_HOST", "");
        job.source         = v.get<std::string>("SOURCE_SE", "");
        job.destination           = v.get<std::string>("DEST_SE", "");
        job.agentDn       = v.get<std::string>("AGENT_DN", "");
        job.submitHost    = v.get<std::string>("SUBMIT_HOST");
        job.userDn        = v.get<std::string>("USER_DN");
        job.userCred      = v.get<std::string>("USER_CRED", "");
        job.credId        = v.get<std::string>("CRED_ID", "");
        job.spaceToken    = v.get<std::string>("SPACE_TOKEN", "");
        job.storageClass  = v.get<std::string>("STORAGE_CLASS", "");
        job.internalJobParams = v.get<std::string>("JOB_PARAMS", "");
        job.overwriteFlag = v.get<std::string>("OVERWRITE_FLAG", "");
        job.sourceSpaceToken = v.get<std::string>("SOURCE_SPACE_TOKEN", "");
        job.sourceSpaceTokenDescription = v.get<std::string>("SOURCE_TOKEN_DESCRIPTION", "");
        job.copyPinLifetime  = static_cast<int>(v.get<double>("COPY_PIN_LIFETIME"));
        job.bringOnline     = static_cast<int>(v.get<double>("BRING_ONLINE"));
        job.checksumMethod = v.get<std::string>("CHECKSUM_METHOD", "");
        job.submitTime     = getTimeT(v, "SUBMIT_TIME");

        try
            {
                job.reuse = v.get<std::string>("REUSE_JOB", "");
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
        file.fileState  = v.get<std::string>("FILE_STATE");
        file.sourceSurl = v.get<std::string>("SOURCE_SURL");
        file.destSurl   = v.get<std::string>("DEST_SURL");
        file.jobId      = v.get<std::string>("JOB_ID");
        file.voName     = v.get<std::string>("VO_NAME");
        file.fileId     = static_cast<int>(v.get<long long>("FILE_ID"));
        file.overwriteFlag   = v.get<std::string>("OVERWRITE_FLAG", "");
        file.userDn          = v.get<std::string>("USER_DN");
        file.credId     = v.get<std::string>("CRED_ID", "");
        file.checksum    = v.get<std::string>("CHECKSUM", "");
        file.checksumMethod    = v.get<std::string>("CHECKSUM_METHOD", "");
        file.sourceSpaceToken = v.get<std::string>("SOURCE_SPACE_TOKEN", "");
        file.destinationSpaceToken   = v.get<std::string>("SPACE_TOKEN", "");
        file.bringOnline   = static_cast<int>(v.get<double>("BRING_ONLINE"),0);
        file.pinLifetime  = static_cast<int>(v.get<double>("COPY_PIN_LIFETIME"),0);
        file.fileMetadata = v.get<std::string>("FILE_METADATA", "");
        file.jobMetadata  = v.get<std::string>("JOB_METADATA", "");
        file.userFilesize = static_cast<double>(v.get<long long>("USER_FILESIZE", 0));
        file.fileIndex    = static_cast<int>(v.get<long long>("FILE_INDEX", 0));
        file.bringOnlineToken = v.get<std::string>("BRINGONLINE_TOKEN", "");
        file.sourceSe = v.get<std::string>("SOURCE_SE", "");
        file.destSe = v.get<std::string>("DEST_SE", "");
        file.selectionStrategy = v.get<std::string>("SELECTION_STRATEGY", "");
        file.internalFileParams = v.get<std::string>("INTERNAL_JOB_PARAMS", "");
        file.userCredentials = v.get<std::string>("USER_CRED", "");

        try
            {
                file.reuseJob = v.get<std::string>("REUSE_JOB", "");
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
        protoConfig.tcpBufferSize = static_cast<int>(v.get<long long>("TCP_BUFFER_SIZE", 0));
        protoConfig.numberOfStreams = static_cast<int>(v.get<double>("NOSTREAMS", 0));
        //protoConfig.unused = static_cast<int>(v.get<long long>("NO_TX_ACTIVITY_TO", 0));
        protoConfig.transferTimeout = static_cast<int>(v.get<long long>("URLCOPY_TX_TO", 0));
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
        transfer.sourceSurl        = v.get<std::string>("SOURCE_SURL");
        transfer.destSurl          = v.get<std::string>("DEST_SURL", "");

        transfer.fileState = v.get<std::string>("FILE_STATE");
        transfer.reason            = v.get<std::string>("REASON", "");
        transfer.numFailures       = static_cast<int>(v.get<double>("RETRY", 0));
        transfer.duration      = v.get<double>("TX_DURATION",0);

        transfer.startTime        = getTimeT(v, "START_TIME");
        transfer.finishTime       = getTimeT(v, "FINISH_TIME");
        transfer.stagingStart     = getTimeT(v, "STAGING_START");
        transfer.stagingFinished    = getTimeT(v, "STAGING_FINISHED");
    }
};

template <>
struct type_conversion<StorageElement>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, StorageElement& se)
    {
        se.endpoint = v.get<std::string>("ENDPOINT", "");
        se.seType  = v.get<std::string>("SE_TYPE", "");
        se.site     = v.get<std::string>("SITE", "");
        se.name     = v.get<std::string>("NAME", "");
        se.state    = v.get<std::string>("STATE", "");
        se.version  = v.get<std::string>("VERSION", "");
        se.host     = v.get<std::string>("HOST", "");
        se.seTransferType     = v.get<std::string>("SE_TRANSFER_TYPE", "");
        se.seTransferProtocol = v.get<std::string>("SE_TRANSFER_PROTOCOL", "");
        se.seControlProtocol  = v.get<std::string>("SE_CONTROL_PROTOCOL", "");
        se.gocdb_id = v.get<std::string>("GOCDB_ID", "");
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
        config.activeTransfers = static_cast<int>(v.get<long long>("ACTIVE", -1));
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
        lnk.symbolicName     = v.get<std::string>("SYMBOLICNAME", "");
        lnk.numberOfStreams         = static_cast<int>(v.get<long long>("NOSTREAMS", 0));
        lnk.tcpBufferSize   = static_cast<int>(v.get<long long>("TCP_BUFFER_SIZE", 0));
        lnk.transferTimeout     = static_cast<int>(v.get<long long>("URLCOPY_TX_TO", 0));
        //lnk.NO_TX_ACTIVITY_TO = static_cast<int>(v.get<long long>("NO_TX_ACTIVITY_TO", 0));
        lnk.autoTuning     = v.get<std::string>("AUTO_TUNING", "");
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

template<>
struct type_conversion<OAuth>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, OAuth& oauth)
    {
        oauth.appKey      = v.get<std::string>("APP_KEY", "");
        oauth.appSecret   = v.get<std::string>("APP_SECRET", "");
        oauth.accessToken = v.get<std::string>("ACCESS_TOKEN", "");
        oauth.accessTokenSecret = v.get<std::string>("ACCESS_TOKEN_SECRET", "");
    }
};

}
