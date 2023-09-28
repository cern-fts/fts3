--
-- FTS3 Schema 8.2.0
-- [FTS-1829] Add SciTags support
--

ALTER TABLE `t_file`
    ADD COLUMN `scitag` int(11) DEFAULT NULL;

ALTER TABLE `t_file_backup`
    ADD COLUMN `scitag` int(11) DEFAULT NULL;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (8, 2, 0, 'FTS-1829: Add SciTags support');
