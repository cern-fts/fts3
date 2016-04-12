--
-- FTS3 Schema 1.1.0
-- [FTS-308] Allow for range settings for the number of actives
-- [FTS-506] Register on the optimizer actual number of actives (Schema change!)
--
-- t_optimize is modified to allow a range of min/max number of actives per link
--

ALTER TABLE t_optimize_active
    ADD min_active INTEGER DEFAULT NULL;
ALTER TABLE t_optimize_active
    ADD max_active INTEGER DEFAULT NULL;

ALTER TABLE t_optimizer_evolution
    ADD actual_active INTEGER DEFAULT NULL;
ALTER TABLE t_optimizer_evolution
    ADD COLUMN queue_size INTEGER DEFAULT NULL;

UPDATE t_optimize_active
    SET min_active = active, max_active = active
    WHERE fixed = 'on' AND min_active IS NULL AND max_active IS NULL;

INSERT INTO t_schema_vers (major, minor, patch, message)
    VALUES (1, 1, 0, 'FTS-308, FTS-506 diff');
