--
-- Script to downgrade from FTS3 Schema 8.1.0 to the previous schema (8.0.1)
--

ALTER TABLE `t_se`
    DROP COLUMN `tpc_support`;
    DROP COLUMN `skip_eviction`;

ALTER TABLE `t_file_retry_errors`
    DROP COLUMN `transfer_host`,
    DROP COLUMN `log_file`;

-- Update schema version number
DELETE FROM t_schema_vers WHERE major = 8 AND minor = 1 AND patch = 0;
UPDATE t_schema_vers SET message = 'Downgrade from 8.1.0' WHERE major = 8 AND minor = 0 AND patch = 1;
