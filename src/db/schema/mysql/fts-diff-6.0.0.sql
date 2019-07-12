--
-- FTS3 Schema 6.0.0
-- Archive stable


ALTER TABLE t_file MODIFY COLUMN file_state enum('STAGING','ARCHIVING','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING') NOT NULL;
ALTER TABLE t_file_backup MODIFY COLUMN file_state enum('STAGING','ARCHIVING','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING') NOT NULL;


ALTER TABLE t_file
    ADD COLUMN `archive_start_time` timestamp NULL DEFAULT NULL,
    ADD COLUMN `archive_finish_time` timestamp NULL DEFAULT NULL;
    
ALTER TABLE t_file_backup
    ADD COLUMN `archive_start_time` timestamp NULL DEFAULT NULL,
    ADD COLUMN `archive_finish_time` timestamp NULL DEFAULT NULL;
    
ALTER TABLE t_job
    ADD COLUMN `archive_timeout` int(11) DEFAULT NULL;
    
        
ALTER TABLE t_job_backup
    ADD COLUMN `archive_timeout` int(11) DEFAULT NULL;
    

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (6, 0, 0, 'FTS-1318 diff');
