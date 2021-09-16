--
-- FTS3 Schema 7.0.0
-- [FTS-1702] Schema changes for destination file report feature
--

ALTER TABLE `t_job`
    ADD COLUMN `dst_file_report` char(1) DEFAULT NULL;

ALTER TABLE `t_job_backup`
    ADD COLUMN `dst_file_report` char(1) DEFAULT NULL;

-- INSERT INTO t_schema_vers (major, minor, patch, message)
-- VALUES (7, 0, 0, 'FTS-1702: Schema changes for destination file report feature');
