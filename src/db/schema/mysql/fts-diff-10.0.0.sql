--
-- FTS3 Schema 10.0.0
-- [FTS-2131] Create database schema v10.0.0
-- [FTS-1925] Full OAuth2 capabilities in FTS for submission, transfers and tape operations
-- [FTS-1990] Implement Token Refresh service in FTS Server
--

ALTER TABLE `t_token`
  ADD COLUMN `marked_for_refresh` tinyint(1) NULL DEFAULT 0,
  ADD COLUMN `refresh_message` varchar(204) NULL DEFAULT NULL,
  ADD COLUMN `refresh_timestamp` timestamp NULL DEFAULT NULL;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (10, 0, 0, 'FTS v3.14.0 schema changes');
