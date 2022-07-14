--
-- FTS3 Schema 8.0.1
-- Remove old or unused tables
--

-- Remove unused "v_staging" view
DROP VIEW IF EXISTS `v_staging`;

-- Remove old tables
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
VALUES (8, 0, 1, 'FTS v3.12.0 schema cleanup');
