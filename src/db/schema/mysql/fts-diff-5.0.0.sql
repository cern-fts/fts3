--
-- FTS3 Schema 5.0.0
-- [FTS-1318] Study FTS schema and propose optimisations
--

ALTER TABLE `t_job` 
	ADD INDEX `idx_jobtype` (`job_type`);
ALTER TABLE `t_file` 
	ADD INDEX `idx_state` (`file_state`);
ALTER TABLE `t_file` 
	ADD INDEX `idx_host` (`transfer_host`);
ALTER TABLE `t_optimizer_evolution` 
	ADD INDEX `idx_datetime` ( `datetime`);

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (5, 0, 0, 'FTS-1318 diff');

