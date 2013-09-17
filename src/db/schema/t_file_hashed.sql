BEGIN;

-- Table to store the fts3 hosts
DROP TABLE IF EXISTS t_hosts;
CREATE TABLE t_hosts (
    hostname    VARCHAR(64) PRIMARY KEY NOT NULL,
    beat        TIMESTAMP NULL DEFAULT NULL
);

-- Dump all finished jobs and files into the backup tables
INSERT INTO t_job_backup
    SELECT * FROM t_job
    WHERE job_finished IS NOT NULL; -- 14 sec for 46K rows
    
INSERT INTO t_file_backup
    SELECT * FROM t_file
    WHERE job_id IN (SELECT job_id FROM t_job_backup); -- 7 min for something more than 1M rows

DELETE FROM t_file WHERE job_id IN (SELECT job_id FROM t_job WHERE job_finished IS NOT NULL); -- 1h 10m for >= 1M rows
DELETE FROM t_job WHERE job_finished IS NOT NULL; -- 17 sec for 46K rows

COMMIT;


BEGIN;
-- Add the new field
ALTER TABLE t_file ADD hashed_id INTEGER UNSIGNED; -- 6 min for 307k rows

-- Populate new field
UPDATE t_file
    SET hashed_id =  CONV(SUBSTRING(MD5(file_id) FROM 1 FOR 8), 16, 10); -- 1 min for 307k

-- Add index
CREATE INDEX file_id_hashed ON t_file(hashed_id); -- 7 min for 307k

COMMIT;

