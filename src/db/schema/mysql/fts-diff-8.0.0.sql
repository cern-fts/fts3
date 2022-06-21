--
-- FTS3 Schema 8.0.0
-- [FTS-1709] Schema changes for SWIFT support
-- [FTS-1744] Schema changes for the eviction feature
-- [FTS-1759] Add primary key on t_schema_vers table
-- [FTS-1768] SRM TURL protocol configurable per link
-- [FTS-1786] Add "t_gridmap" table
-- [FTS-1804] Add "staging_metadata" field to "t_file" table
-- [FTS-1805] Remove "v_staging" view
--

ALTER TABLE `t_se`
    ADD COLUMN `eviction` char(1) DEFAULT NULL;

ALTER TABLE `t_link_config`
    ADD COLUMN `3rd_party_turl` varchar(150) DEFAULT NULL;

ALTER TABLE `t_job`
    ADD COLUMN `os_project_id` varchar(512) DEFAULT NULL;

ALTER TABLE `t_job_backup`
    ADD COLUMN `os_project_id` varchar(512) DEFAULT NULL;

CREATE TABLE `t_gridmap` (
    `dn` VARCHAR(255) NOT NULL,
    `vo` VARCHAR(100) NOT NULL,
    PRIMARY KEY(`dn`)
);

ALTER TABLE `t_schema_vers`
    ADD PRIMARY KEY(`major`, `minor`, `patch`);

ALTER TABLE `t_file`
    ADD COLUMN `staging_metadata` text;

ALTER TABLE `t_file_backup`
    ADD COLUMN `staging_metadata` text;

DROP VIEW IF EXISTS `v_staging`;

--
-- Cleanup
--

DROP TABLE IF EXISTS t_vo_acl;
DROP TABLE IF EXISTS t_group_members;
DROP TABLE IF EXISTS t_se_old;
DROP TABLE IF EXISTS t_link_config_old;
DROP TABLE IF EXISTS t_debug;
DROP TABLE IF EXISTS t_optimize;
DROP TABLE IF EXISTS t_optimize_mode;
DROP TABLE IF EXISTS t_optimize_streams;
DROP TABLE IF EXISTS t_optimize_active;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (8, 0, 0, 'FTS v3.12.0 schema changes');
