--
-- FTS3 Schema 8.1.0
-- [FTS-1914] Schema changes for TPC-support configuration per SE
--

ALTER TABLE `t_se`
    ADD COLUMN `tpc_support` VARCHAR(10) DEFAULT NULL;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (8, 1, 0, 'FTS-1914: Schema changes for TCP-support configuration per SE');
