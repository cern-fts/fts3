--
-- FTS3 Schema 4.0.1
-- [FTS-1042] t_credential.termination_time should not have the "ON UPDATE" clause
--

ALTER TABLE t_credential
    MODIFY COLUMN termination_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (4, 0, 1, 'FTS-1042 diff');
