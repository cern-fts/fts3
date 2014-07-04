DELETE FROM t_job_backup;
DELETE FROM t_dm_backup;
DELETE FROM t_dm;
DELETE FROM t_file_backup;
DELETE FROM t_stage_req;
DELETE FROM t_file;
DELETE FROM t_job_share_config;
DELETE FROM t_job;
DELETE FROM t_vo_acl;
DELETE FROM t_se_pair_acl;
DELETE FROM t_bad_dns;
DELETE FROM t_bad_ses;
DELETE FROM t_share_config;
DELETE FROM t_link_config;
DELETE FROM t_group_members;
DELETE FROM t_se_acl;
DELETE FROM t_se;
DELETE FROM t_credential;
DELETE FROM t_credential_cache;
DELETE FROM t_debug;
DELETE FROM t_config_audit;
DELETE FROM t_optimize;
DELETE FROM t_optimizer_evolution;
DELETE FROM t_optimize_mode;
DELETE FROM t_server_config;
DELETE FROM T_FILE_SHARE_CONFIG;
DELETE FROM t_turl;
INSERT INTO t_server_config (retry,max_time_queue) values(0,0);
INSERT INTO t_server_sanity
    (revertToSubmitted, cancelWaitingFiles, revertNotUsedFiles, forceFailTransfers, setToFailOldQueuedJobs, checkSanityState, cleanUpRecords, msgcron,
     t_revertToSubmitted, t_cancelWaitingFiles, t_revertNotUsedFiles, t_forceFailTransfers, t_setToFailOldQueuedJobs, t_checkSanityState, t_cleanUpRecords, t_msgcron)
VALUES (0, 0, 0, 0, 0, 0, 0, 0,
        CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);
