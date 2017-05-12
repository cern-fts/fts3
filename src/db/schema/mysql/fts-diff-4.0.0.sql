--
-- Storage configuration
--
RENAME TABLE t_se TO t_se_old;
CREATE TABLE t_se (
    `storage`       VARCHAR(150) NOT NULL PRIMARY KEY,
    `site`          VARCHAR(45),
    `metadata`      TEXT NULL,
    `ipv6`          BOOL NULL,
    `udt`           BOOL NULL,
    `debug_level`   INT NULL,
    `inbound_max_active`        INT,
    `inbound_max_throughput`    FLOAT,
    `outbound_max_active`       INT,
    `outbound_max_throughput`   FLOAT
)
AS
    SELECT o.source_se AS storage, NULL AS site, NULL AS metadata,
        o.ipv6 = 'on' AS ipv6, o.udt = 'on' AS udt, d.debug_level AS debug_level,
        NULL AS inbound_max_active, NULL AS inbound_max_throughput,
        NULL AS outbound_max_active, NULL AS outbound_max_throughput
    FROM t_optimize o LEFT JOIN t_debug d ON o.source_se = d.source_se
    WHERE o.source_se IS NOT NULL
UNION
    SELECT o.dest_se, NULL, NULL, o.ipv6 = 'on', o.udt = 'on', d.debug_level, NULL, NULL, NULL, NULL
    FROM t_optimize o LEFT JOIN t_debug d ON o.dest_se = d.dest_se
    WHERE o.dest_se IS NOT NULL
UNION
    SELECT d.source_se, NULL, NULL, NULL, NULL, d.debug_level, NULL, NULL, NULL, NULL
    FROM t_debug d
    WHERE d.source_se IS NOT NULL AND d.source_se != ""
UNION
    SELECT d.dest_se, NULL, NULL, NULL, NULL, d.debug_level, NULL, NULL, NULL, NULL
    FROM t_debug d
    WHERE d.dest_se IS NOT NULL AND d.dest_se != ""
UNION
    (SELECT '*', NULL, NULL, NULL, NULL, NULL, max_per_se, NULL, max_per_se, NULL
    FROM t_server_config
    WHERE vo_name IN (NULL, '*', '')
    LIMIT 1);

--
-- Link configuration
--
RENAME TABLE t_link_config TO t_link_config_old;
CREATE TABLE t_link_config (
    source_se       VARCHAR(150) NOT NULL,
    dest_se         VARCHAR(150) NOT NULL,
    symbolic_name   VARCHAR(150) NOT NULL UNIQUE,
    min_active      INT NULL,
    max_active      INT NULL,
    optimizer_mode  INT NULL,
    tcp_buffer_size INT,
    nostreams       INT,
    PRIMARY KEY(source_se, dest_se)
)
AS
    SELECT o.source_se AS source_se, o.dest_se AS dest_se, CONCAT(source_se, "-", dest_se) AS symbolic_name,
        o.min_active AS min_active, o.max_active AS max_active, om.mode_opt AS optimizer_mode,
        l.tcp_buffer_size AS tcp_buffer_size, l.nostreams AS nostreams
    FROM t_optimize_mode om, t_optimize_active o
    LEFT JOIN t_link_config_old l ON l.source = o.source_se AND l.destination = o.dest_se
UNION
    (SELECT l.source, l.destination, l.symbolicName, NULL, NULL, NULL, l.tcp_buffer_size, l.nostreams
    FROM t_link_config_old l
    WHERE NOT EXISTS (SELECT 1 FROM t_optimize_active oa WHERE oa.source_se = l.source AND oa.dest_se = l.destination)
    )
UNION
    (SELECT '*', '*', '*', 2, s.max_per_link, om.mode_opt, NULL, s.global_tcp_stream
    FROM t_optimize_mode om, t_server_config s
    WHERE vo_name IN (NULL, '*', '')
    LIMIT 1);

--
-- Optimizer status
--
CREATE TABLE t_optimizer (
    `source_se` VARCHAR(150) NOT NULL,
    `dest_se`   VARCHAR(150) NOT NULL,
    `datetime`  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    `ema`       DOUBLE DEFAULT 0,
    `active`    INT DEFAULT 2,
    `nostreams` INT DEFAULT 1,
    PRIMARY KEY(`source_se`, `dest_se`)
)
AS
    SELECT
        o.source_se, o.dest_se, o.datetime, o.ema, o.active, s.nostreams
    FROM t_optimize_active o
    LEFT JOIN t_optimize_streams s ON s.source_se = o.source_se and s.dest_se = o.dest_se;

--
-- Drop obsolete fields
--
ALTER TABLE t_server_config
    DROP COLUMN max_per_link,
    DROP COLUMN max_per_se,
    DROP COLUMN global_tcp_stream;

--
-- Drop link between t_file and t_share_config
--
DROP TABLE t_file_share_config;
ALTER TABLE t_share_config
    DROP FOREIGN KEY t_share_config_fk;
ALTER TABLE t_share_config
    ADD CONSTRAINT t_share_config_fk FOREIGN KEY (source, destination) REFERENCES t_link_config (source_se, dest_se);

--
-- Cleanup
--
-- DROP TABLE t_group_members;
-- DROP TABLE t_se_old;
-- DROP TABLE t_link_config_old;
-- DROP TABLE t_debug;
-- DROP TABLE t_optimize_mode;
-- DROP TABLE t_optimize;
-- DROP TABLE t_optimizer_streams;
-- DROP TABLE t_optimize_active;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (4, 0, 0, 'FTS-894 diff');
