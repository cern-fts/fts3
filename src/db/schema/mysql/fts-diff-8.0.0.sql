--
-- FTS3 Schema 8.0.0
-- [FTS-1744] Schema changes for the eviction feature
--

ALTER TABLE `t_se`
    ADD COLUMN `eviction` char(1) DEFAULT NULL;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (8, 0, 0, 'FTS-1702: Schema changes for the eviction feature');
