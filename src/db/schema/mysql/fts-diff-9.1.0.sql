--
-- FTS3 Schema 9.1.0
-- [FTS-2007] "overwrite_disk_enabled" flag in SE config for "overwrite-when-only-on-disk" feature
--

ALTER TABLE `t_se`
    ADD COLUMN `overwrite_disk_enabled` char(1) DEFAULT NULL;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (9, 1, 0, 'FTS-2007: Implement "overwrite-when-only-on-disk" feature');
