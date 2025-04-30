--
-- Script to downgrade from FTS3 Schema 10.0.0 to the previous schema (9.1.0)
--

ALTER TABLE `t_token`
  DROP COLUMN `marked_for_refresh`,
  DROP COLUMN `refresh_message`,
  DROP COLUMN `refresh_timestamp`,
  DROP COLUMN `unmanaged`,
  CHANGE COLUMN `exchange_retry_timestamp` `retry_timestamp` timestamp NULL DEFAULT NULL,
  CHANGE COLUMN `exchange_retry_delay_m` `retry_delay_m` int unsigned NULL DEFAULT 0,
  CHANGE COLUMN `exchange_attempts` `attempts` int unsigned NULL DEFAULT 0,
  DROP INDEX `idx_retired`;

ALTER TABLE `t_token`
  ADD COLUMN `access_token_not_before` timestamp NOT NULL,
  ADD COLUMN `access_token_refresh_after` timestamp NOT NULL,
      ADD INDEX `idx_retired_access_token_refresh_after` (`retired`, `access_token_refresh_after`);

ALTER TABLE `t_token_provider`
  DROP COLUMN `required_submission_scope`,
  DROP COLUMN `vo_mapping`;

ALTER TABLE `t_file`
  DROP INDEX `idx_staging_token`;

DROP TABLE IF EXISTS `t_webmon_overview_cache`;
DROP TABLE IF EXISTS `t_webmon_overview_cache_control`;

-- Update schema version number
DELETE FROM t_schema_vers WHERE major = 10 AND minor = 0 AND patch = 0;
REPLACE INTO t_schema_vers (major, minor, patch, message) VALUES (9, 1, 0, 'Downgrade from 10.0.0');
