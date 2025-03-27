--
-- FTS3 Schema 9.2.0
-- [FTS-2132] Index to optimize the staging requests count query
--

ALTER TABLE `t_file`
    ADD INDEX `idx_staging_token` (`file_state`, `vo_name`, `source_se`, `bringonline_token`);

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (9, 2, 0, 'FTS-2132: Index to optimize the staging requests count query');
