--
-- Script to downgrade from FTS3 Schema 8.2.0 to the previous schema (8.1.0)
--

ALTER TABLE `t_file`
    DROP COLUMN `scitag`;

ALTER TABLE `t_file_backup`
    DROP COLUMN `scitag`;

-- Update schema version number
DELETE FROM t_schema_vers WHERE major = 8 AND minor = 2 AND patch = 0;
UPDATE t_schema_vers SET message = 'Downgrade from 8.2.0' WHERE major = 8 AND minor = 1 AND patch = 0;
