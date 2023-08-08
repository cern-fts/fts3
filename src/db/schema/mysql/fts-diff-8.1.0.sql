--
-- FTS3 Schema 8.1.0
-- [FTS-1914] TPC-support configuration per SE
-- [FTS-1920] Retry log file
-- [FTS-1923] SkipEviction config option per SE
--

ALTER TABLE `t_se`
    ADD COLUMN `tpc_support` VARCHAR(10) DEFAULT NULL,
    ADD COLUMN `skip_eviction` CHAR(1) DEFAULT NULL;

ALTER TABLE `t_file_retry_errors`
    ADD COLUMN `transfer_host` varchar(255) DEFAULT NULL,
    ADD COLUMN `log_file` varchar(2048) DEFAULT NULL;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (8, 1, 0, 'FTS-1914: TPC-support configuration per SE');
