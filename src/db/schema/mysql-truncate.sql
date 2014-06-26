TRUNCATE t_job_backup;
TRUNCATE t_file_backup;
TRUNCATE t_dm_backup;
TRUNCATE t_dm;
TRUNCATE t_stage_req;
TRUNCATE t_file;
TRUNCATE t_file_share_config;
TRUNCATE t_job;
TRUNCATE t_vo_acl;
TRUNCATE t_se_pair_acl;
TRUNCATE t_bad_dns;
TRUNCATE t_bad_ses;
TRUNCATE t_share_config;
TRUNCATE t_link_config;
TRUNCATE t_group_members;
TRUNCATE t_se_acl;
TRUNCATE t_se;
TRUNCATE t_credential;
TRUNCATE t_credential_cache;
TRUNCATE t_debug;
TRUNCATE t_config_audit;
TRUNCATE t_optimize;
TRUNCATE t_optimize_mode;
TRUNCATE t_optimizer_evolution;
TRUNCATE t_server_config;
TRUNCATE t_server_sanity;
TRUNCATE t_turl;
INSERT INTO t_server_config (retry,max_time_queue) values(0,0);
INSERT INTO t_server_sanity
    (revertToSubmitted, cancelWaitingFiles, revertNotUsedFiles, forceFailTransfers, setToFailOldQueuedJobs, checkSanityState, cleanUpRecords, msgcron,
     t_revertToSubmitted, t_cancelWaitingFiles, t_revertNotUsedFiles, t_forceFailTransfers, t_setToFailOldQueuedJobs, t_checkSanityState, t_cleanUpRecords, t_msgcron)
VALUES (0, 0, 0, 0, 0, 0, 0, 0,
        UTC_TIMESTAMP(), UTC_TIMESTAMP(), UTC_TIMESTAMP(), UTC_TIMESTAMP(), UTC_TIMESTAMP(), UTC_TIMESTAMP(), UTC_TIMESTAMP(), UTC_TIMESTAMP());

