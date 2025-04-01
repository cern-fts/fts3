--
-- FTS3 Schema 10.0.0
-- [FTS-2131] Create database schema v10.0.0
-- [FTS-1925] Full OAuth2 capabilities in FTS for submission, transfers and tape operations
-- [FTS-1990] Implement Token Refresh service in FTS Server
-- [FTS-2044] Allow the FTS systems to skip the token lifecycle management for certain tokens
-- [FTS-2132] Index to optimize the staging requests count query
--

ALTER TABLE `t_token`
  ADD COLUMN `marked_for_refresh` tinyint(1) NULL DEFAULT 0,
  ADD COLUMN `refresh_message` varchar(2048) NULL DEFAULT NULL,
  ADD COLUMN `refresh_timestamp` timestamp NULL DEFAULT NULL,
  ADD COLUMN `unmanaged` tinyint(1) NULL DEFAULT 0;

ALTER TABLE `t_token_provider`
  ADD COLUMN `required_submission_scope` varchar(255) NULL DEFAULT NULL,
  ADD COLUMN `vo_mapping` varchar(100) NULL DEFAULT NULL;

ALTER TABLE `t_file`
    ADD INDEX `idx_staging_token` (`file_state`, `vo_name`, `source_se`, `bringonline_token`);

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (10, 0, 0, 'FTS v3.14.0 schema changes');
