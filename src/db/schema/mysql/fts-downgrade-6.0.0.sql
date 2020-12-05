--
-- Script to downgrade from FTS3 Schema 6.0.0 to the previous schema, 5.0.0
-- 

ALTER TABLE `t_job` 
	DROP COLUMN `target_qos`,
	DROP COLUMN `archive_timeout`;
ALTER TABLE `t_job` 
	MODIFY COLUMN job_state enum('STAGING','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','FINISHEDDIRTY','CANCELED','DELETE') NOT NULL;

ALTER TABLE `t_job_backup` 
	MODIFY COLUMN job_state enum('STAGING','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','FINISHEDDIRTY','CANCELED','DELETE') NOT NULL;
ALTER TABLE `t_job_backup` 
	DROP COLUMN `target_qos`,
	DROP COLUMN `archive_timeout`;

ALTER TABLE `t_file`
    DROP COLUMN `archive_start_time`,
    DROP COLUMN `archive_finish_time`;
ALTER TABLE `t_file` 
	MODIFY COLUMN file_state enum('STAGING','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING') NOT NULL;

ALTER TABLE `t_file_backup`
    DROP COLUMN `archive_start_time`,
    DROP COLUMN `archive_finish_time`;
ALTER TABLE `t_file_backup` 
	MODIFY COLUMN file_state enum('STAGING','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING') NOT NULL;

DROP TABLE t_oauth2_providers;

---
--- Manage foreign key on `cloudStorage_name` before changing var length
---
ALTER TABLE `t_cloudStorageUser`
    DROP FOREIGN KEY `t_cloudStorageUser_ibfk_1`,
    MODIFY `cloudStorage_name` varchar(36) NOT NULL;

ALTER TABLE `t_cloudStorage`
    MODIFY `cloudStorage_name` varchar(50) NOT NULL;

ALTER TABLE `t_cloudStorageUser`
    ADD CONSTRAINT `t_cloudStorageUser_ibfk_1` FOREIGN KEY (`cloudStorage_name`) REFERENCES `t_cloudStorage` (`cloudStorage_name`);

ALTER TABLE `t_link_config`
    DROP COLUMN `no_delegation`;

ALTER TABLE `t_server_config`
    DROP COLUMN `no_streaming`;

UPDATE t_schema_vers SET MAJOR=5, PATCH=0, MESSAGE='DOWNGRADE from 6.0.0' where MAJOR = 6;
