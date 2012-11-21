/**
 * Conversion from and into SOCI
 */
#pragma once

#include <Cred.h>
#include <CredCache.h>
#include <FileTransferStatus.h>
#include <JobStatus.h>
#include <Se.h>
#include <SeAndConfig.h>
#include <SeGroup.h>
#include <SeProtocolConfig.h>
#include <TransferJobs.h>
#include <soci.h>
#include <time.h>

namespace soci
{
    template <>
    struct type_conversion<Cred> {
        typedef values base_type;

        static void from_base(values const& v, indicator, Cred& cred) {
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
    struct type_conversion<CredCache> {
        typedef values base_type;

        static void from_base(values const& v, indicator, CredCache& ccache) {
            ccache.DN                 = v.get<std::string>("dn");
            ccache.certificateRequest = v.get<std::string>("cert_request");
            ccache.delegationID       = v.get<std::string>("dlg_id");
            ccache.privateKey         = v.get<std::string>("priv_key");
            ccache.vomsAttributes     = v.get<std::string>("voms_attrs", std::string());
        }
    };

    template <>
    struct type_conversion<TransferJobs> {
        typedef values base_type;

        static void from_base(values const& v, indicator, TransferJobs& job) {
            struct tm aux_tm;

            job.JOB_ID         = v.get<std::string>("job_id");
            job.JOB_STATE      = v.get<std::string>("job_state");
            job.VO_NAME        = v.get<std::string>("vo_name");
            job.PRIORITY       = v.get<int>("priority");
            job.SUBMIT_HOST    = v.get<std::string>("submit_host", "");
            job.SOURCE         = v.get<std::string>("source", "");
            job.DEST           = v.get<std::string>("dest", "");
            job.AGENT_DN       = v.get<std::string>("agent_dn", "");
            job.SUBMIT_HOST    = v.get<std::string>("submit_host");
            job.SOURCE_SE      = v.get<std::string>("source_se");
            job.DEST_SE        = v.get<std::string>("dest_se");
            job.USER_DN        = v.get<std::string>("user_dn");
            job.USER_CRED      = v.get<std::string>("user_cred");
            job.CRED_ID        = v.get<std::string>("cred_id");
            job.SPACE_TOKEN    = v.get<std::string>("space_token", "");
            job.STORAGE_CLASS  = v.get<std::string>("storage_class", "");
            job.INTERNAL_JOB_PARAMS = v.get<std::string>("job_params");
            job.OVERWRITE_FLAG = v.get<std::string>("overwrite_flag");
            job.SOURCE_SPACE_TOKEN = v.get<std::string>("source_space_token", "");
            job.SOURCE_TOKEN_DESCRIPTION = v.get<std::string>("source_token_description", "");
            job.COPY_PIN_LIFETIME  = v.get<int>("copy_pin_lifetime");
            job.CHECKSUM_METHOD    = v.get<std::string>("checksum_method");
            aux_tm = v.get<struct tm>("submit_time");
            job.SUBMIT_TIME = timegm(&aux_tm);

            if (v.get_indicator("finish_time") == soci::i_ok) {
                aux_tm = v.get<struct tm>("finish_time");
                job.FINISH_TIME = timegm(&aux_tm);
            }
            else {
                job.FINISH_TIME = -1;
            }
        }
    };

    template <>
    struct type_conversion<TransferFiles> {
        typedef values base_type;

        static void from_base(values const& v, indicator, TransferFiles& file) {
            file.FILE_STATE  = v.get<std::string>("file_state");
            file.SOURCE_SURL = v.get<std::string>("source_surl");
            file.DEST_SURL   = v.get<std::string>("dest_surl");
            file.JOB_ID      = v.get<std::string>("job_id");
            file.VO_NAME     = v.get<std::string>("vo_name");
            file.FILE_ID     = v.get<int>("file_id");
            file.OVERWRITE   = v.get<std::string>("overwrite_flag");
            file.DN          = v.get<std::string>("user_dn");
            file.CRED_ID     = v.get<std::string>("cred_id");
            file.CHECKSUM    = v.get<std::string>("checksum");
            file.CHECKSUM_METHOD    = v.get<std::string>("checksum_method");
            file.SOURCE_SPACE_TOKEN = v.get<std::string>("source_space_token");
            file.DEST_SPACE_TOKEN   = v.get<std::string>("space_token");
            file.REASON             = v.get<std::string>("reason", "");

            long long size = v.get<long long>("filesize", 0);
            std::ostringstream str;
            str << size;
            file.FILESIZE = str.str();
        }
    };

    template <>
    struct type_conversion<SeAndConfig> {
        typedef values base_type;

        static void from_base(values const& v, indicator, SeAndConfig& sc) {
            sc.SE_NAME     = v.get<std::string>("se_name");
            sc.SHARE_ID    = v.get<std::string>("share_id");
            sc.SHARE_TYPE  = v.get<std::string>("share_type");
            sc.SHARE_VALUE = v.get<std::string>("share_value");
        }
    };

    template <>
    struct type_conversion<SeProtocolConfig> {
        typedef values base_type;

        static void from_base(values const& v, indicator, SeProtocolConfig& protoConfig) {
            protoConfig.TCP_BUFFER_SIZE = v.get<int>("tcp_buffer_size", 0);
            protoConfig.NOSTREAMS = v.get<int>("nostreams", 0);
            protoConfig.NO_TX_ACTIVITY_TO = v.get<int>("no_tx_activity_to", 0);           
            protoConfig.URLCOPY_TX_TO = v.get<int>("URLCOPY_TX_TO", 0);            
        }
    };

    template <>
    struct type_conversion<JobStatus> {
        typedef values base_type;

        static void from_base(values const& v, indicator, JobStatus& job) {
            struct tm aux_tm;
            job.jobID      = v.get<std::string>("job_id");
            job.jobStatus  = v.get<std::string>("job_state");
            job.clientDN   = v.get<std::string>("user_dn");
            job.reason     = v.get<std::string>("reason", "");
            aux_tm         = v.get<struct tm>("submit_time");
            job.submitTime = timegm(&aux_tm);
            job.priority   = v.get<int>("priority");
            job.voName     = v.get<std::string>("vo_name");

            try {
                job.fileStatus = v.get<std::string>("file_state");
            } catch (...) {
                // Ignore failures, since not all methods ask for this (i.e. listRequests)
            }
        }
    };

    template <>
    struct type_conversion<FileTransferStatus> {
        typedef values base_type;

        static void from_base(values const& v, indicator, FileTransferStatus& transfer) {
            struct tm aux_tm;
            transfer.sourceSURL        = v.get<std::string>("source_surl");
            transfer.destSURL          = v.get<std::string>("dest_surl");
            transfer.transferFileState = v.get<std::string>("file_state");
            transfer.reason            = v.get<std::string>("reason", "");
            aux_tm = v.get<struct tm>("start_time");
            transfer.start_time = timegm(&aux_tm);
            if (v.get_indicator("finish_time") == soci::i_ok) {
                aux_tm = v.get<struct tm>("finish_time");
                transfer.finish_time = timegm(&aux_tm);
            }
            else {
                transfer.finish_time = -1;
            }
        }
    };

    template <>
    struct type_conversion<Se> {
        typedef values base_type;

        static void from_base(values const& v, indicator, Se& se) {
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
    struct type_conversion<SeConfig> {
        typedef values base_type;

        static void from_base(values const& v, indicator, SeConfig& config) {
            config.source     = v.get<std::string>("source");
            config.destination    = v.get<std::string>("dest");
            config.vo  = v.get<std::string>("vo");
            config.symbolicName = v.get<std::string>("symbolicName");
            config.state = v.get<std::string>("state");	    
        }

    };

    template <>
    struct type_conversion<SeGroup> {
      typedef values base_type;

      static void from_base(values const& v, indicator, SeGroup& grp) {
          grp.active       = v.get<int>("active", -1);
          grp.groupName    = v.get<std::string>("groupName");
          grp.member       = v.get<std::string>("member");
          grp.symbolicName = v.get<std::string>("symbolicName");
      }
    };
}
