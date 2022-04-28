--
-- FTS3 Schema 8.0.0
-- [FTS-1744] Schema changes for the eviction feature
-- [FTS-1768] SRM TURL protocol configurable per link
-- [FTS01786] Add t_gripdmap table
--

ALTER TABLE `t_se`
    ADD COLUMN `eviction` char(1) DEFAULT NULL;

ALTER TABLE `t_link_config`
    ADD COLUMN `3rd_party_turl` varchar(150) DEFAULT NULL;

CREATE TABLE `t_gridmap` (
    `dn` VARCHAR(255) NOT NULL,
    `vo` VARCHAR(100) NOT NULL,
    PRIMARY KEY(`dn`)
);

ALTER TABLE `t_schema_vers`
    ADD PRIMARY KEY(`major`, `minor`, `patch`);

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (8, 0, 0, 'FTS-1702: Schema changes for the eviction feature');
