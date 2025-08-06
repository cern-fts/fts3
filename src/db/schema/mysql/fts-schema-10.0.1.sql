-- MySQL dump 10.13  Distrib 8.0.36, for Linux (x86_64)
--
-- Host: dbod-fts-dev.cern.ch    Database: fts_schema_10_0_0
-- ------------------------------------------------------
-- Server version	8.4.2

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `t_activity_share_config`
--

DROP TABLE IF EXISTS `t_activity_share_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_activity_share_config` (
  `vo` varchar(100) NOT NULL,
  `activity_share` varchar(1024) NOT NULL,
  `active` varchar(3) DEFAULT NULL,
  PRIMARY KEY (`vo`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_authz_dn`
--

DROP TABLE IF EXISTS `t_authz_dn`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_authz_dn` (
  `dn` varchar(255) NOT NULL,
  `operation` varchar(64) NOT NULL,
  PRIMARY KEY (`dn`,`operation`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_bad_dns`
--

DROP TABLE IF EXISTS `t_bad_dns`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_bad_dns` (
  `dn` varchar(255) NOT NULL DEFAULT '',
  `message` varchar(2048) DEFAULT NULL,
  `addition_time` timestamp NULL DEFAULT NULL,
  `admin_dn` varchar(255) DEFAULT NULL,
  `status` varchar(10) DEFAULT NULL,
  `wait_timeout` int DEFAULT '0',
  PRIMARY KEY (`dn`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_bad_ses`
--

DROP TABLE IF EXISTS `t_bad_ses`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_bad_ses` (
  `se` varchar(256) NOT NULL DEFAULT '',
  `message` varchar(2048) DEFAULT NULL,
  `addition_time` timestamp NULL DEFAULT NULL,
  `admin_dn` varchar(255) DEFAULT NULL,
  `vo` varchar(100) DEFAULT NULL,
  `status` varchar(10) DEFAULT NULL,
  `wait_timeout` int DEFAULT '0',
  PRIMARY KEY (`se`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_cloudStorage`
--

DROP TABLE IF EXISTS `t_cloudStorage`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_cloudStorage` (
  `cloudStorage_name` varchar(150) NOT NULL,
  `app_key` varchar(255) DEFAULT NULL,
  `app_secret` varchar(255) DEFAULT NULL,
  `service_api_url` varchar(1024) DEFAULT NULL,
  PRIMARY KEY (`cloudStorage_name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_cloudStorageUser`
--

DROP TABLE IF EXISTS `t_cloudStorageUser`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_cloudStorageUser` (
  `user_dn` varchar(700) NOT NULL DEFAULT '',
  `vo_name` varchar(100) NOT NULL DEFAULT '',
  `cloudStorage_name` varchar(150) NOT NULL,
  `access_token` varchar(255) DEFAULT NULL,
  `access_token_secret` varchar(255) DEFAULT NULL,
  `request_token` varchar(255) DEFAULT NULL,
  `request_token_secret` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`user_dn`,`vo_name`,`cloudStorage_name`),
  KEY `cloudStorage_name` (`cloudStorage_name`),
  CONSTRAINT `t_cloudStorageUser_ibfk_1` FOREIGN KEY (`cloudStorage_name`) REFERENCES `t_cloudStorage` (`cloudStorage_name`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_config_audit`
--

DROP TABLE IF EXISTS `t_config_audit`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_config_audit` (
  `datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `dn` varchar(255) DEFAULT NULL,
  `config` varchar(4000) DEFAULT NULL,
  `action` varchar(100) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_credential`
--

DROP TABLE IF EXISTS `t_credential`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_credential` (
  `dlg_id` char(16) NOT NULL,
  `dn` varchar(255) NOT NULL,
  `proxy` longtext,
  `voms_attrs` longtext,
  `termination_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`dlg_id`,`dn`),
  KEY `termination_time` (`termination_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_credential_cache`
--

DROP TABLE IF EXISTS `t_credential_cache`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_credential_cache` (
  `dlg_id` char(16) NOT NULL,
  `dn` varchar(255) NOT NULL,
  `cert_request` longtext,
  `priv_key` longtext,
  `voms_attrs` longtext,
  PRIMARY KEY (`dlg_id`,`dn`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_dm`
--

DROP TABLE IF EXISTS `t_dm`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_dm` (
  `file_id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `job_id` char(36) NOT NULL,
  `file_state` varchar(32) NOT NULL,
  `dmHost` varchar(150) DEFAULT NULL,
  `source_surl` varchar(900) DEFAULT NULL,
  `dest_surl` varchar(900) DEFAULT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `error_scope` varchar(32) DEFAULT NULL,
  `error_phase` varchar(32) DEFAULT NULL,
  `reason` varchar(2048) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci DEFAULT NULL,
  `checksum` varchar(100) DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `start_time` timestamp NULL DEFAULT NULL,
  `internal_file_params` varchar(255) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `pid` int DEFAULT NULL,
  `tx_duration` double DEFAULT NULL,
  `retry` int DEFAULT '0',
  `user_filesize` double DEFAULT NULL,
  `file_metadata` varchar(255) DEFAULT NULL,
  `activity` varchar(255) DEFAULT 'default',
  `selection_strategy` varchar(255) DEFAULT NULL,
  `dm_start` timestamp NULL DEFAULT NULL,
  `dm_finished` timestamp NULL DEFAULT NULL,
  `dm_token` varchar(255) DEFAULT NULL,
  `retry_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timeout` int DEFAULT NULL,
  `hashed_id` int unsigned DEFAULT '0',
  `vo_name` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`file_id`),
  KEY `dm_job_id` (`job_id`),
  CONSTRAINT `fk_dmjob_id` FOREIGN KEY (`job_id`) REFERENCES `t_job` (`job_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB AUTO_INCREMENT=545755 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_dm_backup`
--

DROP TABLE IF EXISTS `t_dm_backup`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_dm_backup` (
  `file_id` bigint unsigned NOT NULL DEFAULT '0',
  `job_id` char(36) NOT NULL,
  `file_state` varchar(32) NOT NULL,
  `dmHost` varchar(150) DEFAULT NULL,
  `source_surl` varchar(900) DEFAULT NULL,
  `dest_surl` varchar(900) DEFAULT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `error_scope` varchar(32) DEFAULT NULL,
  `error_phase` varchar(32) DEFAULT NULL,
  `reason` varchar(2048) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci DEFAULT NULL,
  `checksum` varchar(100) DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `start_time` timestamp NULL DEFAULT NULL,
  `internal_file_params` varchar(255) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `pid` int DEFAULT NULL,
  `tx_duration` double DEFAULT NULL,
  `retry` int DEFAULT '0',
  `user_filesize` double DEFAULT NULL,
  `file_metadata` varchar(255) DEFAULT NULL,
  `activity` varchar(255) DEFAULT 'default',
  `selection_strategy` varchar(255) DEFAULT NULL,
  `dm_start` timestamp NULL DEFAULT NULL,
  `dm_finished` timestamp NULL DEFAULT NULL,
  `dm_token` varchar(255) DEFAULT NULL,
  `retry_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timeout` int DEFAULT NULL,
  `hashed_id` int unsigned DEFAULT '0',
  `vo_name` varchar(100) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file`
--

DROP TABLE IF EXISTS `t_file`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_file` (
  `log_file_debug` tinyint(1) DEFAULT NULL,
  `file_id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `file_index` int DEFAULT NULL,
  `job_id` char(36) NOT NULL,
  `file_state` enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING','FORCE_START','TOKEN_PREP') NOT NULL,
  `transfer_host` varchar(255) DEFAULT NULL,
  `source_surl` varchar(1100) DEFAULT NULL,
  `dest_surl` varchar(1100) DEFAULT NULL,
  `source_se` varchar(255) DEFAULT NULL,
  `dest_se` varchar(255) DEFAULT NULL,
  `staging_host` varchar(1024) DEFAULT NULL,
  `reason` varchar(2048) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci DEFAULT NULL,
  `current_failures` int DEFAULT NULL,
  `filesize` bigint DEFAULT NULL,
  `checksum` varchar(100) DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `start_time` timestamp NULL DEFAULT NULL,
  `internal_file_params` varchar(255) DEFAULT NULL,
  `pid` int DEFAULT NULL,
  `tx_duration` double DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `retry` int DEFAULT '0',
  `user_filesize` bigint DEFAULT NULL,
  `file_metadata` text,
  `selection_strategy` char(32) DEFAULT NULL,
  `staging_start` timestamp NULL DEFAULT NULL,
  `staging_finished` timestamp NULL DEFAULT NULL,
  `bringonline_token` varchar(255) DEFAULT NULL,
  `retry_timestamp` timestamp NULL DEFAULT NULL,
  `log_file` varchar(2048) DEFAULT NULL,
  `t_log_file_debug` int DEFAULT NULL,
  `hashed_id` int unsigned DEFAULT '0',
  `vo_name` varchar(50) DEFAULT NULL,
  `activity` varchar(255) DEFAULT 'default',
  `transferred` bigint DEFAULT '0',
  `priority` int DEFAULT '3',
  `dest_surl_uuid` char(36) DEFAULT NULL,
  `archive_start_time` timestamp NULL DEFAULT NULL,
  `archive_finish_time` timestamp NULL DEFAULT NULL,
  `staging_metadata` text,
  `archive_metadata` text,
  `scitag` int DEFAULT NULL,
  `src_token_id` char(16) DEFAULT NULL,
  `dst_token_id` char(16) DEFAULT NULL,
  `file_state_initial` char(32) DEFAULT NULL,
  PRIMARY KEY (`file_id`),
  UNIQUE KEY `dest_surl_uuid` (`dest_surl_uuid`),
  KEY `idx_job_id` (`job_id`),
  KEY `idx_activity` (`vo_name`,`activity`),
  KEY `idx_link_state_vo` (`source_se`,`dest_se`,`file_state`,`vo_name`),
  KEY `idx_finish_time` (`finish_time`),
  KEY `idx_staging` (`file_state`,`vo_name`,`source_se`),
  KEY `idx_state_host` (`file_state`,`transfer_host`),
  KEY `idx_state` (`file_state`),
  KEY `idx_host` (`transfer_host`),
  KEY `src_token_id` (`src_token_id`),
  KEY `dst_token_id` (`dst_token_id`),
  KEY `idx_staging_token` (`file_state`,`vo_name`,`source_se`,`bringonline_token`),
  KEY `idx_link_state_finish_time` (`source_se`,`dest_se`,`file_state`,`finish_time`),
  CONSTRAINT `dst_token_id` FOREIGN KEY (`dst_token_id`) REFERENCES `t_token` (`token_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `job_id` FOREIGN KEY (`job_id`) REFERENCES `t_job` (`job_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `src_token_id` FOREIGN KEY (`src_token_id`) REFERENCES `t_token` (`token_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB AUTO_INCREMENT=8872390197 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file_backup`
--

DROP TABLE IF EXISTS `t_file_backup`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_file_backup` (
  `log_file_debug` tinyint(1) DEFAULT NULL,
  `file_id` bigint unsigned NOT NULL DEFAULT '0',
  `file_index` int DEFAULT NULL,
  `job_id` char(36) NOT NULL,
  `file_state` enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED','ON_HOLD','ON_HOLD_STAGING','FORCE_START','TOKEN_PREP') NOT NULL,
  `transfer_host` varchar(255) DEFAULT NULL,
  `source_surl` varchar(1100) DEFAULT NULL,
  `dest_surl` varchar(1100) DEFAULT NULL,
  `source_se` varchar(255) DEFAULT NULL,
  `dest_se` varchar(255) DEFAULT NULL,
  `staging_host` varchar(1024) DEFAULT NULL,
  `reason` varchar(2048) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci DEFAULT NULL,
  `current_failures` int DEFAULT NULL,
  `filesize` bigint DEFAULT NULL,
  `checksum` varchar(100) DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `start_time` timestamp NULL DEFAULT NULL,
  `internal_file_params` varchar(255) DEFAULT NULL,
  `pid` int DEFAULT NULL,
  `tx_duration` double DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `retry` int DEFAULT '0',
  `user_filesize` bigint DEFAULT NULL,
  `file_metadata` text,
  `selection_strategy` char(32) DEFAULT NULL,
  `staging_start` timestamp NULL DEFAULT NULL,
  `staging_finished` timestamp NULL DEFAULT NULL,
  `bringonline_token` varchar(255) DEFAULT NULL,
  `retry_timestamp` timestamp NULL DEFAULT NULL,
  `log_file` varchar(2048) DEFAULT NULL,
  `t_log_file_debug` int DEFAULT NULL,
  `hashed_id` int unsigned DEFAULT '0',
  `vo_name` varchar(50) DEFAULT NULL,
  `activity` varchar(255) DEFAULT 'default',
  `transferred` bigint DEFAULT '0',
  `priority` int DEFAULT '3',
  `dest_surl_uuid` char(36) DEFAULT NULL,
  `archive_start_time` timestamp NULL DEFAULT NULL,
  `archive_finish_time` timestamp NULL DEFAULT NULL,
  `staging_metadata` text,
  `archive_metadata` text,
  `scitag` int DEFAULT NULL,
  `src_token_id` char(16) DEFAULT NULL,
  `dst_token_id` char(16) DEFAULT NULL,
  `file_state_initial` char(32) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file_retry_errors`
--

DROP TABLE IF EXISTS `t_file_retry_errors`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_file_retry_errors` (
  `file_id` bigint unsigned NOT NULL,
  `attempt` int NOT NULL,
  `datetime` timestamp NULL DEFAULT NULL,
  `reason` varchar(2048) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci DEFAULT NULL,
  `transfer_host` varchar(255) DEFAULT NULL,
  `log_file` varchar(2048) DEFAULT NULL,
  PRIMARY KEY (`file_id`,`attempt`),
  KEY `idx_datetime` (`datetime`),
  CONSTRAINT `t_file_retry_errors_ibfk_1` FOREIGN KEY (`file_id`) REFERENCES `t_file` (`file_id`) ON DELETE CASCADE ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_gridmap`
--

DROP TABLE IF EXISTS `t_gridmap`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_gridmap` (
  `dn` varchar(255) NOT NULL,
  `vo` varchar(100) NOT NULL,
  PRIMARY KEY (`dn`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_hosts`
--

DROP TABLE IF EXISTS `t_hosts`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_hosts` (
  `hostname` varchar(64) NOT NULL,
  `beat` timestamp NULL DEFAULT NULL,
  `drain` int DEFAULT '0',
  `service_name` varchar(64) NOT NULL,
  PRIMARY KEY (`hostname`,`service_name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_job`
--

DROP TABLE IF EXISTS `t_job`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_job` (
  `job_id` char(36) NOT NULL,
  `job_state` enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','FINISHEDDIRTY','CANCELED','DELETE') NOT NULL,
  `job_type` char(1) DEFAULT NULL,
  `cancel_job` char(1) DEFAULT NULL,
  `source_se` varchar(255) DEFAULT NULL,
  `dest_se` varchar(255) DEFAULT NULL,
  `user_dn` varchar(1024) DEFAULT NULL,
  `cred_id` char(16) DEFAULT NULL,
  `vo_name` varchar(50) DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  `submit_time` timestamp NULL DEFAULT NULL,
  `priority` int DEFAULT '3',
  `submit_host` varchar(255) DEFAULT NULL,
  `max_time_in_queue` int DEFAULT NULL,
  `space_token` varchar(255) DEFAULT NULL,
  `internal_job_params` varchar(255) DEFAULT NULL,
  `overwrite_flag` char(1) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `source_space_token` varchar(255) DEFAULT NULL,
  `copy_pin_lifetime` int DEFAULT NULL,
  `checksum_method` char(1) DEFAULT NULL,
  `bring_online` int DEFAULT NULL,
  `retry` int DEFAULT '0',
  `retry_delay` int DEFAULT '0',
  `target_qos` varchar(255) DEFAULT NULL,
  `job_metadata` text,
  `archive_timeout` int DEFAULT NULL,
  `dst_file_report` char(1) DEFAULT NULL,
  `os_project_id` varchar(512) DEFAULT NULL,
  PRIMARY KEY (`job_id`),
  KEY `idx_vo_name` (`vo_name`),
  KEY `idx_jobfinished` (`job_finished`),
  KEY `idx_link` (`source_se`,`dest_se`),
  KEY `idx_submission` (`submit_time`,`submit_host`),
  KEY `idx_jobtype` (`job_type`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_job_backup`
--

DROP TABLE IF EXISTS `t_job_backup`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_job_backup` (
  `job_id` char(36) NOT NULL,
  `job_state` enum('STAGING','ARCHIVING','QOS_TRANSITION','QOS_REQUEST_SUBMITTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','FINISHEDDIRTY','CANCELED','DELETE') NOT NULL,
  `job_type` char(1) DEFAULT NULL,
  `cancel_job` char(1) DEFAULT NULL,
  `source_se` varchar(255) DEFAULT NULL,
  `dest_se` varchar(255) DEFAULT NULL,
  `user_dn` varchar(1024) DEFAULT NULL,
  `cred_id` char(16) DEFAULT NULL,
  `vo_name` varchar(50) DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  `submit_time` timestamp NULL DEFAULT NULL,
  `priority` int DEFAULT '3',
  `submit_host` varchar(255) DEFAULT NULL,
  `max_time_in_queue` int DEFAULT NULL,
  `space_token` varchar(255) DEFAULT NULL,
  `internal_job_params` varchar(255) DEFAULT NULL,
  `overwrite_flag` char(1) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `source_space_token` varchar(255) DEFAULT NULL,
  `copy_pin_lifetime` int DEFAULT NULL,
  `checksum_method` char(1) DEFAULT NULL,
  `bring_online` int DEFAULT NULL,
  `retry` int DEFAULT '0',
  `retry_delay` int DEFAULT '0',
  `target_qos` varchar(255) DEFAULT NULL,
  `job_metadata` text,
  `archive_timeout` int DEFAULT NULL,
  `dst_file_report` char(1) DEFAULT NULL,
  `os_project_id` varchar(512) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_link_config`
--

DROP TABLE IF EXISTS `t_link_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_link_config` (
  `source_se` varchar(150) NOT NULL,
  `dest_se` varchar(150) NOT NULL,
  `symbolic_name` varchar(150) NOT NULL,
  `min_active` int DEFAULT NULL,
  `max_active` int DEFAULT NULL,
  `optimizer_mode` int DEFAULT NULL,
  `tcp_buffer_size` int DEFAULT NULL,
  `nostreams` int DEFAULT NULL,
  `no_delegation` varchar(3) DEFAULT NULL,
  `3rd_party_turl` varchar(150) DEFAULT NULL,
  PRIMARY KEY (`source_se`,`dest_se`),
  UNIQUE KEY `symbolic_name` (`symbolic_name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

INSERT INTO `t_link_config` (source_se, dest_se, symbolic_name, min_active, max_active, optimizer_mode, nostreams, no_delegation)
VALUES ('*', '*', '*', 2, 130, 2, 0, 'off');

--
-- Table structure for table `t_oauth2_apps`
--

DROP TABLE IF EXISTS `t_oauth2_apps`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_oauth2_apps` (
  `client_id` varchar(64) NOT NULL,
  `client_secret` varchar(128) NOT NULL,
  `owner` varchar(1024) NOT NULL,
  `name` varchar(128) NOT NULL,
  `description` varchar(512) DEFAULT NULL,
  `website` varchar(1024) DEFAULT NULL,
  `redirect_to` varchar(4096) DEFAULT NULL,
  PRIMARY KEY (`client_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_oauth2_codes`
--

DROP TABLE IF EXISTS `t_oauth2_codes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_oauth2_codes` (
  `client_id` varchar(64) DEFAULT NULL,
  `code` varchar(128) NOT NULL,
  `scope` varchar(512) DEFAULT NULL,
  `dlg_id` varchar(100) NOT NULL,
  PRIMARY KEY (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_oauth2_providers`
--

DROP TABLE IF EXISTS `t_oauth2_providers`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_oauth2_providers` (
  `provider_url` varchar(250) NOT NULL,
  `provider_jwk` varchar(1000) NOT NULL,
  PRIMARY KEY (`provider_url`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_oauth2_tokens`
--

DROP TABLE IF EXISTS `t_oauth2_tokens`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_oauth2_tokens` (
  `client_id` varchar(64) NOT NULL,
  `scope` varchar(512) DEFAULT NULL,
  `access_token` varchar(128) DEFAULT NULL,
  `token_type` varchar(64) DEFAULT NULL,
  `expires` datetime DEFAULT NULL,
  `refresh_token` varchar(128) DEFAULT NULL,
  `dlg_id` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`client_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_optimizer`
--

DROP TABLE IF EXISTS `t_optimizer`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_optimizer` (
  `source_se` varchar(150) NOT NULL,
  `dest_se` varchar(150) NOT NULL,
  `datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `ema` double DEFAULT '0',
  `active` int DEFAULT '2',
  `nostreams` int DEFAULT '1',
  PRIMARY KEY (`source_se`,`dest_se`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_optimizer_evolution`
--

DROP TABLE IF EXISTS `t_optimizer_evolution`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_optimizer_evolution` (
  `datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `active` int DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `success` float DEFAULT NULL,
  `rationale` text,
  `diff` int DEFAULT '0',
  `actual_active` int DEFAULT NULL,
  `queue_size` int DEFAULT NULL,
  `ema` double DEFAULT NULL,
  `filesize_avg` double DEFAULT NULL,
  `filesize_stddev` double DEFAULT NULL,
  KEY `idx_optimizer_evolution` (`source_se`,`dest_se`,`datetime`),
  KEY `idx_datetime` (`datetime`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_schema_vers`
--

DROP TABLE IF EXISTS `t_schema_vers`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_schema_vers` (
  `major` int NOT NULL,
  `minor` int NOT NULL,
  `patch` int NOT NULL,
  `message` text,
  PRIMARY KEY (`major`,`minor`,`patch`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

INSERT INTO `t_schema_vers` (major, minor, patch, message)
VALUES (10, 0, 0, 'Schema 10.0.0');

--
-- Table structure for table `t_se`
--

DROP TABLE IF EXISTS `t_se`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_se` (
  `storage` varchar(150) NOT NULL,
  `site` varchar(45) DEFAULT NULL,
  `metadata` text,
  `ipv6` tinyint(1) DEFAULT NULL,
  `udt` tinyint(1) DEFAULT NULL,
  `debug_level` int DEFAULT NULL,
  `inbound_max_active` int DEFAULT NULL,
  `inbound_max_throughput` float DEFAULT NULL,
  `outbound_max_active` int DEFAULT NULL,
  `outbound_max_throughput` float DEFAULT NULL,
  `eviction` char(1) DEFAULT NULL,
  `tpc_support` varchar(10) DEFAULT NULL,
  `skip_eviction` char(1) DEFAULT NULL,
  `tape_endpoint` char(1) DEFAULT NULL,
  `overwrite_disk_enabled` char(1) DEFAULT NULL,
  PRIMARY KEY (`storage`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

INSERT INTO `t_se` (storage, inbound_max_active, outbound_max_active)
VALUES ('*', 200, 200);

--
-- Table structure for table `t_server_config`
--

DROP TABLE IF EXISTS `t_server_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_server_config` (
  `retry` int DEFAULT '0',
  `max_time_queue` int DEFAULT '0',
  `sec_per_mb` int DEFAULT '0',
  `global_timeout` int DEFAULT '0',
  `vo_name` varchar(100) DEFAULT NULL,
  `no_streaming` varchar(3) DEFAULT NULL,
  `show_user_dn` varchar(3) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

INSERT INTO `t_server_config` (vo_name)
VALUES ('*');

--
-- Table structure for table `t_share_config`
--

DROP TABLE IF EXISTS `t_share_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_share_config` (
  `source` varchar(150) NOT NULL,
  `destination` varchar(150) NOT NULL,
  `vo` varchar(100) NOT NULL,
  `active` int NOT NULL,
  PRIMARY KEY (`source`,`destination`,`vo`),
  CONSTRAINT `t_share_config_fk` FOREIGN KEY (`source`, `destination`) REFERENCES `t_link_config` (`source_se`, `dest_se`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_stage_req`
--

DROP TABLE IF EXISTS `t_stage_req`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_stage_req` (
  `vo_name` varchar(100) NOT NULL,
  `host` varchar(150) NOT NULL,
  `operation` varchar(150) NOT NULL,
  `concurrent_ops` int DEFAULT '0',
  PRIMARY KEY (`vo_name`,`host`,`operation`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_token`
--

DROP TABLE IF EXISTS `t_token`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_token` (
  `token_id` char(16) NOT NULL,
  `access_token` longtext NOT NULL,
  `access_token_expiry` timestamp NOT NULL,
  `refresh_token` longtext,
  `issuer` varchar(1024) NOT NULL,
  `scope` varchar(1024) NOT NULL,
  `audience` varchar(1024) NOT NULL,
  `exchange_retry_timestamp` timestamp NULL DEFAULT NULL,
  `exchange_retry_delay_m` int unsigned DEFAULT '0',
  `exchange_attempts` int unsigned DEFAULT '0',
  `exchange_message` varchar(2048) DEFAULT NULL,
  `retired` tinyint(1) NOT NULL DEFAULT '0',
  `marked_for_refresh` tinyint(1) DEFAULT '0',
  `refresh_message` varchar(2048) DEFAULT NULL,
  `refresh_timestamp` timestamp NULL DEFAULT NULL,
  `unmanaged` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`token_id`),
  KEY `fk_token_issuer` (`issuer`),
  KEY `idx_retired` (`retired`),
  CONSTRAINT `fk_token_issuer` FOREIGN KEY (`issuer`) REFERENCES `t_token_provider` (`issuer`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_token_provider`
--

DROP TABLE IF EXISTS `t_token_provider`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_token_provider` (
  `name` varchar(255) NOT NULL,
  `issuer` varchar(1024) NOT NULL,
  `client_id` varchar(255) NOT NULL,
  `client_secret` varchar(255) NOT NULL,
  `required_submission_scope` varchar(255) DEFAULT NULL,
  `vo_mapping` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`issuer`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_webmon_overview_cache`
--

DROP TABLE IF EXISTS `t_webmon_overview_cache`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_webmon_overview_cache` (
  `count` int NOT NULL,
  `file_state` varchar(32) NOT NULL,
  `source_se` varchar(150) NOT NULL,
  `dest_se` varchar(150) NOT NULL,
  `vo_name` varchar(100) NOT NULL,
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`file_state`,`source_se`,`dest_se`,`vo_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_webmon_overview_cache_control`
--

DROP TABLE IF EXISTS `t_webmon_overview_cache_control`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `t_webmon_overview_cache_control` (
  `id` int NOT NULL,
  `update_duration` double DEFAULT NULL,
  `updated_at` timestamp NULL DEFAULT NULL,
  `update_host` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2025-04-25 14:00:00
