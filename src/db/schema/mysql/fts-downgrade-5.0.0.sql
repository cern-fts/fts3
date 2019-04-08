--
-- Script to downgrade from FTS3 Schema 5.0.0 to the previous schema
-- 

ALTER TABLE `t_job` 
	DROP INDEX `idx_jobtype`;
ALTER TABLE `t_file` 
	DROP INDEX `idx_state`;
ALTER TABLE `t_file` 
	DROP INDEX `idx_host`;
ALTER TABLE `t_optimizer_evolution` 
	DROP INDEX `idx_datetime`;
ALTER TABLE `t_file`
        MODIFY reason varchar(2048) CHARACTER SET latin1;
ALTER TABLE `t_file_backup`
        MODIFY reason varchar(2048) CHARACTER SET latin1;
ALTER TABLE `t_dm`
        MODIFY reason varchar(2048) CHARACTER SET latin1;
ALTER TABLE `t_dm_backup`
        MODIFY reason varchar(2048) CHARACTER SET latin1;
ALTER TABLE `t_file_retry_errors`
        MODIFY reason varchar(2048) CHARACTER SET latin1;
ALTER TABLE `t_file_retry_errors`
        DROP INDEX `idx_datetime`;
ALTER TABLE `t_file` 
	DROP `priority`;
ALTER TABLE `t_file` 
       DROP UNIQUE KEY `dest_surl_uuid`;
ALTER TABLE `t_file`
       DROP `dest_surl_uuid`;

DROP TABLE t_dm;
RENAME TABLE t_dm_old TO t_dm;

ALTER TABLE t_dm
    DROP INDEX dm_job_id;
    
--
-- Archive tables need to match the new schema
--
DROP TABLE t_dm_backup;
RENAME TABLE t_dm_backup_old TO t_dm_backup;

UPDATE t_schema_vers SET MAJOR=4, PATCH=1, MESSAGE='DOWNGRADE from 5.0.0' where MAJOR = 5;
