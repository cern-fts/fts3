SET default_storage_engine=InnoDB;

-- Per https://gitlab.cern.ch/fts/fts3/blob/develop/src/db/schema/unused.md
-- These tables are fairly small, so an in-place modification is reasonable
ALTER TABLE t_optimize
    DROP COLUMN `file_id`,
    DROP COLUMN `timeout`,
    DROP COLUMN `buffer`,
    DROP COLUMN `filesize`;

ALTER TABLE t_link_config
    DROP COLUMN `placeholder1`,
    DROP COLUMN `placeholder2`,
    DROP COLUMN `placeholder3`,
    DROP COLUMN `NO_TX_ACTIVITY_TO`;

-- Reduce size of dlg_id
CREATE TABLE t_credential_cache_new (
    `dlg_id`        CHAR(16) NOT NULL,
    `dn`            VARCHAR(255),
    `cert_request`  LONGTEXT,
    `priv_key`      LONGTEXT,
    `voms_attrs`    LONGTEXT,
    PRIMARY KEY (dlg_id, dn)
)
AS
SELECT dlg_id, dn, cert_request, priv_key, voms_attrs
FROM t_credential_cache;

RENAME TABLE t_credential_cache TO t_credential_cache_old;
RENAME TABLE t_credential_cache_new TO t_credential_cache;

CREATE TABLE t_credential_new (
    `dlg_id`        CHAR(16) NOT NULL,
    `dn`            VARCHAR(255),
    `proxy`         LONGTEXT,
    `voms_attrs`    LONGTEXT,
    `termination_time`  TIMESTAMP NOT NULL,
    PRIMARY KEY (dlg_id, dn),
    INDEX (termination_time)
)
AS
SELECT dlg_id, dn, proxy, voms_attrs, termination_time
FROM t_credential;

RENAME TABLE t_credential TO t_credential_old;
RENAME TABLE t_credential_new TO t_credential;

-- DROP TABLE t_credential_cache_old;
-- DROP TABLE t_credential_old;

-- Unused + FTS-599 + FTS-617
-- These tables can be quite big, so an in-place modification would take
-- a long time.
-- Thus, we update the schema in two steps:
-- * Replicate a modified table with the data cloned
-- * Add indexes

-- t_job
-- t_file has a foreign key on this, so need to do it first.
-- Dropped fields
-- * job_params varchar(255), unused
-- * agent_dn varchar(1024), unused
-- * user_cred varchar(255), pointless
-- * voms_cred longtext, depends on cred_id
-- * storage_class varchar(255), unused
-- * myproxy_server varchar(255), unused
-- * source_token_description varchar(255), unused
-- * fail_nearline char(1), unused
-- * configuration_count (int), unused
-- * finish_time (timestamp), redundant with job_finished

CREATE TABLE t_job_new (
  `job_id`              CHAR(36) NOT NULL,
  `job_state`           ENUM(
    'STAGING', 'SUBMITTED', 'READY', 'ACTIVE', 'FINISHED', 'FAILED', 'FINISHEDDIRTY', 'CANCELED', 'DELETE'
  ) NOT NULL,                                   -- Was job_state varchar(32)
  `job_type`            CHAR(1) DEFAULT NULL,   -- Was reuse_job varchar(3)
  `cancel_job`          CHAR(1) DEFAULT NULL,
  `source_se`           VARCHAR(255) DEFAULT NULL,
  `dest_se`             VARCHAR(255) DEFAULT NULL,
  `user_dn`             VARCHAR(1024) DEFAULT NULL,
  `cred_id`             CHAR(16) DEFAULT NULL,  -- Was cred_id varchar(100), actually 16 hex digits are stored
  `vo_name`             VARCHAR(50) DEFAULT NULL,
  `reason`              VARCHAR(2048) DEFAULT NULL,
  `submit_time`         TIMESTAMP NULL DEFAULT NULL,
  `priority`            INT(11) DEFAULT '3',
  `submit_host`         VARCHAR(255) DEFAULT NULL,
  `max_time_in_queue`   INT(11) DEFAULT NULL,
  `space_token`         VARCHAR(255) DEFAULT NULL,
  `internal_job_params` VARCHAR(255) DEFAULT NULL,
  `overwrite_flag`      CHAR(1) DEFAULT NULL,
  `job_finished`        TIMESTAMP NULL DEFAULT NULL,
  `source_space_token`  VARCHAR(255) DEFAULT NULL,
  `copy_pin_lifetime`   INT(11) DEFAULT NULL,
  `checksum_method`     CHAR(1) DEFAULT NULL,   -- Was checksum_method varchar(10), char(1) already in reality
  `bring_online`        INT(11) DEFAULT NULL,
  `retry`               INT(11) DEFAULT '0',
  `retry_delay`         INT(11) DEFAULT '0',
  `job_metadata`        TEXT,                   -- Was varchar(1024), already hit problems before because of size
  PRIMARY KEY (`job_id`)
)
AS
SELECT job_id, job_state, reuse_job AS job_type, cancel_job,
    source_se, dest_se, user_dn, cred_id, vo_name, reason,
    submit_time, priority, submit_host,
    max_time_in_queue, space_token, internal_job_params,
    overwrite_flag, job_finished, source_space_token, copy_pin_lifetime,
    checksum_method, bring_online, retry, retry_delay, job_metadata
FROM t_job;

RENAME TABLE t_job TO t_job_old;
RENAME TABLE t_job_new TO t_job;

ALTER TABLE t_job
    ADD INDEX idx_vo_name (vo_name),
    ADD INDEX idx_jobfinished (job_finished),
    ADD INDEX idx_link (source_se, dest_se),
    ADD INDEX idx_submission (submit_time, submit_host);

-- t_file
-- Dropped:
-- * logical_name VARCHAR(1100)
-- * symbolicName VARCHAR(255)
-- * error_scope  VARCHAR(32)
-- * error_phase  VARCHAR(32)
-- * reason_class VARCHAR(32)
-- * num_failures INT
-- * catalog_failures INT
-- * prestage_failures INT
-- * job_finished TIMESTAMP (redundant with t_job.job_finished)
CREATE TABLE t_file_new (
  `file_id`             BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT, -- Was file_id INT(11), as of today, 31 bits are required for a file_id
  `file_index`          INT(11) DEFAULT NULL,
  `job_id`              CHAR(36) NOT NULL,
  `file_state`          ENUM(
    'STAGING', 'STARTED', 'SUBMITTED', 'READY', 'ACTIVE', 'FINISHED', 'FAILED', 'CANCELED', 'NOT_USED'
  ) NOT NULL,          -- Was file_state VARCHAR(32)
  `transfer_host`       VARCHAR(255) DEFAULT NULL,  -- Was transferHost
  `source_surl`         VARCHAR(1100) DEFAULT NULL,
  `dest_surl`           VARCHAR(1100) DEFAULT NULL,
  `source_se`           VARCHAR(255) DEFAULT NULL,
  `dest_se`             VARCHAR(255) DEFAULT NULL,
  `staging_host`        VARCHAR(1024) DEFAULT NULL,
  `reason`              VARCHAR(2048) DEFAULT NULL,
  `current_failures`    INT(11) DEFAULT NULL,
  `filesize`            BIGINT DEFAULT NULL,        -- Was filesize DOUBLE
  `checksum`            VARCHAR(100) DEFAULT NULL,
  `finish_time`         TIMESTAMP NULL DEFAULT NULL,
  `start_time`          TIMESTAMP NULL DEFAULT NULL,
  `internal_file_params` VARCHAR(255) DEFAULT NULL,
  `pid`                 INT(11) DEFAULT NULL,
  `tx_duration`         DOUBLE DEFAULT NULL,
  `throughput`          FLOAT DEFAULT NULL,
  `retry`               INT(11) DEFAULT '0',
  `user_filesize`       BIGINT DEFAULT NULL,        -- Was DOUBLE
  `file_metadata`       TEXT,                       -- Was file_metadata VARCHAR(1024)
  `selection_strategy`  CHAR(32) DEFAULT NULL,      -- Was VARCHAR(255)
  `staging_start`       TIMESTAMP NULL DEFAULT NULL,
  `staging_finished`    TIMESTAMP NULL DEFAULT NULL,
  `bringonline_token`   VARCHAR(255) DEFAULT NULL,
  `retry_timestamp`     TIMESTAMP NULL DEFAULT NULL,
  `wait_timestamp`      TIMESTAMP NULL DEFAULT NULL,
  `wait_timeout`        INT(11) DEFAULT NULL,
  `log_file`            VARCHAR(2048) DEFAULT NULL,
  `log_file_debug`      TINYINT(1) DEFAULT NULL,   -- Was INT
  `hashed_id`           INT(10) unsigned DEFAULT '0',
  `vo_name`             VARCHAR(50) DEFAULT NULL,
  `activity`            VARCHAR(255) DEFAULT 'default',
  `transferred`         BIGINT DEFAULT '0',         -- Was DOUBLE
  CONSTRAINT `job_id` FOREIGN KEY (`job_id`) REFERENCES `t_job` (`job_id`)
)
AS
SELECT file_id, file_index, job_id, file_state,
    transferHost AS transfer_host, source_surl, dest_surl, source_se, dest_se,
    agent_dn AS staging_host, reason, current_failures, filesize, checksum,
    finish_time, start_time, internal_file_params,
    pid, tx_duration, throughput, retry, user_filesize,
    file_metadata, selection_strategy, staging_start, staging_finished,
    bringonline_token, retry_timestamp, wait_timestamp, wait_timeout,
    t_log_file AS log_file, t_log_file_debug AS t_log_file_debug, hashed_id, vo_name, activity, transferred
FROM t_file;

RENAME TABLE t_file TO t_file_old;
RENAME TABLE t_file_new TO t_file;

ALTER TABLE t_file
    ADD INDEX idx_job_id (job_id),
    ADD INDEX idx_waittimeout (wait_timeout),
    ADD INDEX idx_activity (vo_name, activity),
    ADD INDEX idx_state_host (file_state, transfer_host),
    ADD INDEX idx_link_state_vo (source_se, dest_se, file_state, vo_name),
    ADD INDEX idx_finish_time (finish_time);

--
-- Need to re-create also t_file_retry_errors pointing to the new table
--
CREATE TABLE t_file_retry_errors_new (
    `file_id`   BIGINT UNSIGNED NOT NULL,
    `attempt`   INTEGER NOT NULL,
    `datetime`  TIMESTAMP NULL DEFAULT NULL,
    `reason`    VARCHAR(2048),
    CONSTRAINT PRIMARY KEY (`file_id`, `attempt`),
    CONSTRAINT FOREIGN KEY (`file_id`) REFERENCES `t_file` (`file_id`) ON DELETE CASCADE
)
AS
SELECT file_id, attempt, datetime, reason
FROM  t_file_retry_errors;

RENAME TABLE t_file_retry_errors TO t_file_retry_errors_old;
RENAME TABLE t_file_retry_errors_new TO t_file_retry_errors;

--
-- Same goes for t_file_share_config
--
CREATE TABLE t_file_share_config_new (
    `file_id`       BIGINT UNSIGNED NOT NULL,
    `source`        VARCHAR(150)   NOT NULL,
    `destination`   VARCHAR(150)   NOT NULL,
    `vo`            VARCHAR(100)   NOT NULL,
    CONSTRAINT PRIMARY KEY (`file_id`, `source`, `destination`, `vo`),
    CONSTRAINT FOREIGN KEY (`source`, `destination`, `vo`) REFERENCES `t_share_config` (`source`, `destination`, `vo`) ON DELETE CASCADE,
    CONSTRAINT FOREIGN KEY (`file_id`) REFERENCES `t_file` (`file_id`) ON DELETE CASCADE
)
AS
SELECT file_id, source, destination, vo
FROM t_file_share_config;

RENAME TABLE t_file_share_config TO t_file_share_config_old;
RENAME TABLE t_file_share_config_new TO t_file_share_config;

--
-- t_dm needs to point to the new t_job
--
ALTER TABLE t_dm
    DROP FOREIGN KEY t_dm_ibfk_1;
ALTER TABLE t_dm
    ADD CONSTRAINT `fk_job_id` FOREIGN KEY (`job_id`) REFERENCES `t_job` (`job_id`);

-- DROP TABLE t_file_share_config_old;
-- DROP TABLE t_file_retry_errors_old;
-- DROP TABLE t_file_old;
-- DROP TABLE t_job_old;

--
-- Unused
--
DROP TABLE t_server_sanity;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (3, 0, 0, 'FTS-599, FTS-815, FTS-824 diff');
