--
-- Script to downgrade from FTS3 Schema 9.0.0 to the previous schema (8.2.0)
--

ALTER TABLE `t_file`
  DROP COLUMN `src_token_id`,
  DROP COLUMN `dst_token_id`,
  DROP COLUMN `file_state_initial`,
  MODIFY COLUMN `file_state` enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING','FORCE_START') NOT NULL,
  DROP CONSTRAINT `src_token_id`,
  DROP CONSTRAINT `dst_token_id`;

ALTER TABLE `t_file_backup`
  DROP COLUMN `src_token_id`,
  DROP COLUMN `dst_token_id`,
  DROP COLUMN `file_state_initial`,
  MODIFY COLUMN `file_state` enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING','FORCE_START') NOT NULL;

ALTER TABLE `t_se`
  DROP COLUMN `tape_endpoint`;

DROP TABLE IF EXISTS `t_token`;
DROP TABLE IF EXISTS `t_token_provider`;

-- Update schema version number
DELETE FROM t_schema_vers WHERE major = 9 AND minor = 0 AND patch = 0;
UPDATE t_schema_vers SET message = 'Downgrade from 9.0.0' WHERE major = 8 AND minor = 2 AND patch = 0;
