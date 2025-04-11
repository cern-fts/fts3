--
-- FTS3 Schema 10.0.0
-- [FTS-2131] Create database schema v10.0.0
-- [FTS-1925] Full OAuth2 capabilities in FTS for submission, transfers and tape operations
-- [FTS-1990] Implement Token Refresh service in FTS Server
-- [FTS-2044] Allow the FTS systems to skip the token lifecycle management for certain tokens
-- [FTS-2132] Index to optimize the staging requests count query
-- [FTS-2156] Backend cache of the Web Monitoring overview page
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

CREATE TABLE `t_webmon_overview_cache` (
  `count` int NOT NULL,
  `file_state` varchar(32) NOT NULL,
  `source_se` varchar(150) NOT NULL,
  `dest_se` varchar(150) NOT NULL,
  `vo_name` varchar(100) NOT NULL,
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`file_state`, `source_se`, `dest_se`, `vo_name`)
);

CREATE TABLE `t_webmon_overview_cache_control` (
  `id` int NOT NULL,
  `update_duration` double NULL DEFAULT NULL,
  `updated_at` timestamp nULL DEFAULT NULL,
  `update_host` varchar(100) NULL DEFAULT NULL,
  PRIMARY KEY (`id`)
);

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (10, 0, 0, 'FTS v3.14.0 schema changes');
