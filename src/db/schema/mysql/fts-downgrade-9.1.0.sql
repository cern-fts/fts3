--
-- Script to downgrade from FTS3 Schema 9.1.0 to the previous schema (9.0.0)
--

ALTER TABLE `t_se`
    DROP COLUMN `overwrite_disk_enabled`;

-- Update schema version number
DELETE FROM t_schema_vers WHERE major = 9 AND minor = 1 AND patch = 0;
UPDATE t_schema_vers SET message = 'Downgrade from 9.1.0' WHERE major = 9 AND minor = 0 AND patch = 0;
