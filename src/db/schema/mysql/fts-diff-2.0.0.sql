--
-- FTS3 Schema 1.1.0
-- [FTS-308] Allow for range settings for the number of actives
-- [FTS-506] Register on the optimizer actual number of actives (Schema change!)
--
-- t_optimize is modified to allow a range of min/max number of actives per link
--

--
-- FTS 308
--

ALTER TABLE t_optimize_active
    ADD COLUMN `min_active` INTEGER DEFAULT NULL,
    ADD COLUMN `max_active` INTEGER DEFAULT NULL,
    DROP COLUMN `message`;

-- Set range for existing fixed links
UPDATE t_optimize_active
    SET min_active = active, max_active = active
    WHERE fixed = 'on' AND min_active IS NULL AND max_active IS NULL;


--
-- Update t_optimizer_evolution in three steps
-- It is normally faster, and keeps data
--

-- First, create an empty one, identical
CREATE TABLE t_optimizer_evolution_new LIKE t_optimizer_evolution;

-- Modify the schema in the empty table
ALTER TABLE t_optimizer_evolution_new
    CHANGE COLUMN `filesize` `success` FLOAT DEFAULT NULL,
    CHANGE COLUMN `agrthroughput` `throughput` FLOAT DEFAULT NULL,
    DROP COLUMN `buffer`,
    DROP COLUMN `nostreams`,
    DROP COLUMN `timeout`,
    ADD COLUMN `ema` FLOAT DEFAULT NULL,
    ADD COLUMN `rationale` TEXT DEFAULT NULL,
    ADD COLUMN `diff` INTEGER DEFAULT 0,
    ADD COLUMN actual_active INTEGER DEFAULT NULL,
    ADD COLUMN queue_size INTEGER DEFAULT NULL,
    ADD COLUMN filesize_avg DOUBLE DEFAULT NULL,
    ADD COLUMN filesize_stddev DOUBLE DEFAULT NULL;

-- Populate the empty table with data from the old one, adapted to the
-- new schema
INSERT INTO t_optimizer_evolution_new
    (datetime, source_se, dest_se,
     throughput_limit, active, throughput, success,
     rationale)
SELECT datetime, source_se, dest_se,
    nostreams AS throughput_limit, active, throughput, filesize AS success,
    'Entry recovered from old database schema'
FROM t_optimizer_evolution
WHERE datetime > (UTC_TIMESTAMP() - INTERVAL 7 DAY);

-- Drop old table, create new one
DROP TABLE t_optimizer_evolution;
RENAME TABLE t_optimizer_evolution_new TO t_optimizer_evolution;

-- Prepare t_optimize_streams
TRUNCATE t_optimize_streams;
ALTER TABLE t_optimize_streams
    DROP COLUMN `throughput`,
    DROP COLUMN `tested`,
    DROP PRIMARY KEY,
    ADD PRIMARY KEY (source_se, dest_se);

--
-- Store update history
--
INSERT INTO t_schema_vers (major, minor, patch, message)
    VALUES (2, 0, 0, 'FTS-308, FTS-506 diff');
