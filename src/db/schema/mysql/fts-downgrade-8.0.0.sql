--
-- Script to downgrade from FTS3 Schema 8.0.0 to the previous schema (7.0.0)
--

ALTER TABLE `t_se`
    DROP COLUMN `eviction`;

ALTER TABLE `t_link_config`
    DROP COLUMN `3rd_party_turl`;

ALTER TABLE `t_job`
    DROP COLUMN `os_project_id`;

ALTER TABLE `t_job_backup`
    DROP COLUMN `os_project_id`;

ALTER TABLE `t_file`
    DROP COLUMN `staging_metadata`,
    DROP COLUMN `archive_metadata`;
ALTER TABLE `t_file`
    MODIFY COLUMN `file_state` enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING') NOT NULL;

ALTER TABLE `t_file_backup`
    DROP COLUMN `staging_metadata`,
    DROP COLUMN `archive_metadata`;
ALTER TABLE `t_file_backup`
    MODIFY COLUMN `file_state` enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING') NOT NULL;

ALTER TABLE `t_schema_vers`
    DROP PRIMARY KEY;

-- Bring back "v_staging" view
CREATE OR REPLACE VIEW `v_staging` AS
    SELECT job_id, file_id, hashed_id, vo_name, source_se, file_state, source_surl
    FROM t_file
    WHERE file_state = 'STAGING';

-- Update schema version number
DELETE FROM t_schema_vers WHERE major = 8 AND minor = 0;
UPDATE t_schema_vers SET message = 'Downgrade from 8.0.0' WHERE major = 7 AND minor = 0;
