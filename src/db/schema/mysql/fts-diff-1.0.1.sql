--
-- FTS3 Schema 1.0.1
-- [FTS-530] Database upgrade tool
--
-- t_schema_vers modified to allow longer descriptions
--

ALTER TABLE t_schema_vers
  DROP COLUMN state;

ALTER TABLE t_schema_vers
  ADD COLUMN message TEXT;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (1, 0, 1, 'FTS-530 diff');
