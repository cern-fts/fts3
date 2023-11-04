--
-- FTS3 Schema 9.0.0
-- [FTS-1928] REST API should accept and validate FTS submission token
--

CREATE TABLE `t_token_provider` (
  `name` varchar(255) NOT NULL,
  `issuer` varchar(1024) NOT NULL,
  `client_id` varchar(255) NOT NULL,
  `client_secret` varchar(255) NOT NULL,
  PRIMARY KEY (`issuer`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `t_token` (
  `token_id` char(16) NOT NULL,
  `access_token` longtext NOT NULL,
  `refresh_token` longtext,
  `issuer` varchar(1024) NOT NULL,
  `scope` varchar(1024) NOT NULL,
  `audience` varchar(1024) NOT NULL,
  PRIMARY KEY (`token_id`),
  CONSTRAINT `fk_token_issuer` FOREIGN KEY (`issuer`) REFERENCES `t_token_provider` (`issuer`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

ALTER TABLE `t_file`
  ADD COLUMN `src_token_id` char(16) DEFAULT NULL,
  ADD COLUMN `dst_token_id` char(16) DEFAULT NULL,
  MODIFY COLUMN `file_state` enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING','FORCE_START','TOKEN_PREP') NOT NULL,
  ADD CONSTRAINT `src_token_id` FOREIGN KEY (`src_token_id`) REFERENCES `t_token` (`token_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  ADD CONSTRAINT `dst_token_id` FOREIGN KEY (`dst_token_id`) REFERENCES `t_token` (`token_id`) ON DELETE RESTRICT ON UPDATE RESTRICT;

ALTER TABLE `t_file_backup`
  ADD COLUMN `src_token_id` char(16) DEFAULT NULL,
  ADD COLUMN `dst_token_id` char(16) DEFAULT NULL,
  MODIFY COLUMN `file_state` enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING','FORCE_START','TOKEN_PREP') NOT NULL;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (9, 0, 0, 'FTS-1925: Full OAuth2 capabilities in FTS for submission, transfers and tape operations');