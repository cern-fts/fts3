--
-- FTS3 Schema 5.0.0
-- [FTS-1318] Study FTS schema and propose optimisations
-- [FTS-1239] Revise the t_dm schema
-- [FTS-1329] Make t_file and t_file_backup reason UTF-8
-- [FTS-1201] De-normalize priority in DB
-- [FTS-1353] Avoid submitting multiple transfers to the same destination 
-- 

ALTER TABLE `t_job` 
	ADD INDEX `idx_jobtype` (`job_type`);
ALTER TABLE `t_file` 
	ADD INDEX `idx_state` (`file_state`);
ALTER TABLE `t_file` 
	ADD INDEX `idx_host` (`transfer_host`);
ALTER TABLE `t_optimizer_evolution` 
	ADD INDEX `idx_datetime` ( `datetime`);
ALTER TABLE `t_file` 
	MODIFY reason varchar(2048) CHARACTER SET utf8;
ALTER TABLE `t_file_backup` 
        MODIFY reason varchar(2048) CHARACTER SET utf8;
ALTER TABLE `t_dm`
        MODIFY reason varchar(2048) CHARACTER SET utf8;
ALTER TABLE `t_dm_backup`
        MODIFY reason varchar(2048) CHARACTER SET utf8;
ALTER TABLE `t_file_retry_errors`
        MODIFY reason varchar(2048) CHARACTER SET utf8;
ALTER TABLE `t_file_retry_errors`
        ADD INDEX `idx_datetime` ( `datetime`);
ALTER TABLE `t_file` 
	ADD COLUMN `priority` int(11) DEFAULT '3';
UPDATE `t_job`
	SET `priority`=3 where `priority`!=3;
ALTER TABLE `t_file` 
       ADD COLUMN `dest_surl_uuid` char(36) DEFAULT NULL;
ALTER TABLE `t_file` 
       ADD UNIQUE KEY `dest_surl_uuid` (`dest_surl_uuid`);

CREATE TABLE `t_dm_new` (
  `file_id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `job_id` char(36) NOT NULL,
  `file_state` varchar(32) NOT NULL,
  `dmHost` varchar(150) DEFAULT NULL,
  `source_surl` varchar(900) DEFAULT NULL,
  `dest_surl` varchar(900) DEFAULT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `error_scope` varchar(32) DEFAULT NULL,
  `error_phase` varchar(32) DEFAULT NULL,
  `reason` varchar(2048) CHARACTER SET utf8 DEFAULT NULL,
  `checksum` varchar(100) DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `start_time` timestamp NULL DEFAULT NULL,
  `internal_file_params` varchar(255) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `pid` int(11) DEFAULT NULL,
  `tx_duration` double DEFAULT NULL,
  `retry` int(11) DEFAULT '0',
  `user_filesize` double DEFAULT NULL,
  `file_metadata` varchar(255) DEFAULT NULL,
  `activity` varchar(255) DEFAULT 'default',
  `selection_strategy` varchar(255) DEFAULT NULL,
  `dm_start` timestamp NULL DEFAULT NULL,
  `dm_finished` timestamp NULL DEFAULT NULL,
  `dm_token` varchar(255) DEFAULT NULL,
  `retry_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timeout` int(11) DEFAULT NULL,
  `hashed_id` int(10) unsigned DEFAULT '0',
  `vo_name` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`file_id`),
  CONSTRAINT `fk_dmjob_id` FOREIGN KEY (`job_id`) REFERENCES `t_job` (`job_id`)
) 
AS
SELECT file_id, job_id, file_state, dmHost, source_surl, dest_surl, source_se, dest_se, NULL as error_scope, NULL as error_phase, reason, checksum, finish_time, start_time, NULL as internal_file_params,
job_finished, NULL as pid, tx_duration, retry, user_filesize, file_metadata, activity, NULL as selection_strategy, NULL as dm_start, NULL as dm_finished, dm_token, retry_timestamp, wait_timestamp, wait_timeout, hashed_id, vo_name
FROM t_dm;

RENAME TABLE t_dm TO t_dm_old;
RENAME TABLE t_dm_new TO t_dm;

ALTER TABLE t_dm
    ADD INDEX dm_job_id (job_id);
    
--
-- Archive tables need to match the new schema
--
RENAME TABLE t_dm_backup TO t_dm_backup_old;
CREATE TABLE t_dm_backup ENGINE = ARCHIVE AS (SELECT * FROM t_dm WHERE NULL);

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (5, 0, 0, 'FTS-1318 diff');
