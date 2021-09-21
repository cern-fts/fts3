--
-- Script to downgrade from FTS3 Schema 7.0.0 to the previous schema (6.0.0)
--

ALTER TABLE `t_job`
	DROP COLUMN `dst_file_report`;

ALTER TABLE `t_job_backup`
	DROP COLUMN `dst_file_report`;

--
-- View for files that are to be staged, but haven't been requested
--
CREATE OR REPLACE VIEW v_staging AS
    SELECT q.job_id, q.file_id, q.hashed_id, q.vo_name, q.source_se, q.file_state, q.source_surl
    FROM t_file q LEFT JOIN t_file s ON
        q.source_surl = s.source_surl AND
        q.vo_name = s.vo_name AND
        s.source_se = q.source_se AND
        s.file_state = 'STARTED'
    WHERE q.file_state = 'STAGING' AND s.file_state IS NULL;

UPDATE t_schema_vers SET major = 6, minor = 0, message = 'Downgrade from 7.0.0' WHERE major = 7;
