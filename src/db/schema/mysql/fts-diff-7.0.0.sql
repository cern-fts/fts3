--
-- FTS3 Schema 7.0.0
-- [FTS-1702] Schema changes for destination file report feature
-- [FTS-1719] Remove "v_staging" database view which avoids bringonline requests for STARTED files
--

ALTER TABLE `t_job`
    ADD COLUMN `dst_file_report` char(1) DEFAULT NULL;

ALTER TABLE `t_job_backup`
    ADD COLUMN `dst_file_report` char(1) DEFAULT NULL;

CREATE OR REPLACE VIEW `v_staging` AS
    SELECT job_id, file_id, hashed_id, vo_name, source_se, file_state, source_surl
    FROM t_file
    WHERE file_state = 'STAGING';

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (7, 0, 0, 'FTS-1702: Schema changes for destination file report feature');
