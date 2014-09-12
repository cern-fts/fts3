drop trigger se_id_info_auto_inc;
drop sequence file_file_id_seq;
drop trigger file_file_id_auto_inc;
drop sequence dm_file_id_seq;
drop trigger dm_file_id_auto_inc;
drop sequence se_id_info_seq;
drop trigger t_optimize_auto_number_inc;
drop sequence t_optimize_auto_number_seq;


DROP TABLE t_turl;
DROP TABLE t_job_backup;
DROP TABLE t_file_backup;
DROP TABLE t_dm_backup;
DROP TABLE t_dm;
DROP TABLE t_schema_vers;
DROP TABLE t_stage_req;
DROP TABLE t_file_share_config;
DROP TABLE t_file_retry_errors;
DROP TABLE t_file;
DROP TABLE t_job;
DROP TABLE t_vo_acl;
DROP TABLE t_se_pair_acl;
DROP TABLE t_bad_dns;
DROP TABLE t_bad_ses;
DROP TABLE t_share_config;
DROP TABLE t_activity_share_config;
DROP TABLE t_link_config;
DROP TABLE t_group_members;
DROP TABLE t_se_acl;
DROP TABLE t_se;
DROP TABLE t_credential_vers;
DROP TABLE t_credential;
DROP TABLE t_credential_cache;
DROP TABLE t_debug;
DROP TABLE t_config_audit;
DROP TABLE t_optimizer_evolution;
DROP TABLE t_optimize;
DROP TABLE t_optimize_mode;
DROP TABLE t_server_config;
DROP TABLE t_profiling_info;
DROP TABLE t_profiling_snapshot;
DROP TABLE t_server_sanity;
DROP TABLE t_hosts;
DROP TABLE t_optimize_streams;
DROP TABLE t_optimize_active;
DROP TABLE t_cloudstorageuser;
DROP TABLE t_cloudstorage;



exit;
