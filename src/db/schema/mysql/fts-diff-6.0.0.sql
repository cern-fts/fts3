--
-- FTS3 Schema 6.0.0
-- Schema changes for QoS daemon and OIDC integration
--


ALTER TABLE `t_file`
    ADD COLUMN `archive_start_time` timestamp NULL DEFAULT NULL,
    ADD COLUMN `archive_finish_time` timestamp NULL DEFAULT NULL;
ALTER TABLE `t_file`
	MODIFY COLUMN file_state enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING') NOT NULL;

ALTER TABLE `t_file_backup`
    ADD COLUMN `archive_start_time` timestamp NULL DEFAULT NULL,
    ADD COLUMN `archive_finish_time` timestamp NULL DEFAULT NULL;
ALTER TABLE `t_file_backup`
	MODIFY COLUMN file_state enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING') NOT NULL;

ALTER TABLE `t_job`
	ADD COLUMN `target_qos` varchar(255),
	ADD COLUMN `archive_timeout` int(11) DEFAULT NULL;
ALTER TABLE `t_job`
	MODIFY COLUMN job_state enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','FINISHEDDIRTY','CANCELED','DELETE') NOT NULL;

ALTER TABLE `t_job_backup`
	ADD COLUMN `target_qos` varchar(255),
	ADD COLUMN `archive_timeout` int(11) DEFAULT NULL;
ALTER TABLE `t_job_backup`
	MODIFY COLUMN job_state enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','FINISHEDDIRTY','CANCELED','DELETE') NOT NULL;

CREATE TABLE t_oauth2_providers (
    `provider_url` VARCHAR(250) NOT NULL,
    `provider_jwk`   VARCHAR(1000) NOT NULL,
    PRIMARY KEY(`provider_url`)
);

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (6, 0, 0, 'QoS daemon and OIDC integration diff');
