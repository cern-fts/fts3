-- MySQL dump 10.13  Distrib 5.1.73, for redhat-linux-gnu (x86_64)
--
-- Host: dbod-fts-dev.cern.ch    Database: fts3
-- ------------------------------------------------------
-- Server version	5.7.15-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
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
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_activity_share_config` (
  `vo` varchar(100) NOT NULL,
  `activity_share` varchar(1024) DEFAULT NULL,
  `active` varchar(3) DEFAULT NULL,
  PRIMARY KEY (`vo`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_authz_dn`
--

DROP TABLE IF EXISTS `t_authz_dn`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
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
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_bad_dns` (
  `dn` varchar(255) NOT NULL DEFAULT '',
  `message` varchar(2048) DEFAULT NULL,
  `addition_time` timestamp NULL DEFAULT NULL,
  `admin_dn` varchar(255) DEFAULT NULL,
  `status` varchar(10) DEFAULT NULL,
  `wait_timeout` int(11) DEFAULT '0',
  PRIMARY KEY (`dn`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_bad_ses`
--

DROP TABLE IF EXISTS `t_bad_ses`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_bad_ses` (
  `se` varchar(256) NOT NULL DEFAULT '',
  `message` varchar(2048) DEFAULT NULL,
  `addition_time` timestamp NULL DEFAULT NULL,
  `admin_dn` varchar(255) DEFAULT NULL,
  `vo` varchar(100) DEFAULT NULL,
  `status` varchar(10) DEFAULT NULL,
  `wait_timeout` int(11) DEFAULT '0',
  PRIMARY KEY (`se`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_cloudStorage`
--

DROP TABLE IF EXISTS `t_cloudStorage`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_cloudStorage` (
  `cloudStorage_name` varchar(50) NOT NULL,
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
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_cloudStorageUser` (
  `user_dn` varchar(700) NOT NULL DEFAULT '',
  `vo_name` varchar(100) NOT NULL DEFAULT '',
  `cloudStorage_name` varchar(36) NOT NULL,
  `access_token` varchar(255) DEFAULT NULL,
  `access_token_secret` varchar(255) DEFAULT NULL,
  `request_token` varchar(255) DEFAULT NULL,
  `request_token_secret` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`user_dn`,`vo_name`,`cloudStorage_name`),
  KEY `cloudStorage_name` (`cloudStorage_name`),
  CONSTRAINT `t_cloudStorageUser_ibfk_1` FOREIGN KEY (`cloudStorage_name`) REFERENCES `t_cloudStorage` (`cloudStorage_name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_config_audit`
--

DROP TABLE IF EXISTS `t_config_audit`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
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
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_credential` (
  `dlg_id` char(16) NOT NULL,
  `dn` varchar(255) NOT NULL,
  `proxy` longtext,
  `voms_attrs` longtext,
  `termination_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`dlg_id`,`dn`),
  KEY `termination_time` (`termination_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_credential_cache`
--

DROP TABLE IF EXISTS `t_credential_cache`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
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
-- Table structure for table `t_credential_cache_old`
--

DROP TABLE IF EXISTS `t_credential_cache_old`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_credential_cache_old` (
  `dlg_id` varchar(100) NOT NULL DEFAULT '',
  `dn` varchar(255) NOT NULL DEFAULT '',
  `cert_request` longtext,
  `priv_key` longtext,
  `voms_attrs` longtext,
  PRIMARY KEY (`dlg_id`,`dn`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_credential_old`
--

DROP TABLE IF EXISTS `t_credential_old`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_credential_old` (
  `dlg_id` varchar(100) NOT NULL DEFAULT '',
  `dn` varchar(255) NOT NULL DEFAULT '',
  `proxy` longtext,
  `voms_attrs` longtext,
  `termination_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`dlg_id`,`dn`),
  KEY `termination_time` (`termination_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_debug`
--

DROP TABLE IF EXISTS `t_debug`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_debug` (
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `debug` varchar(3) DEFAULT 'off',
  `debug_level` int(11) DEFAULT '1'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_dm`
--

DROP TABLE IF EXISTS `t_dm`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_dm` (
  `file_id` int(11) NOT NULL AUTO_INCREMENT,
  `job_id` char(36) NOT NULL,
  `file_state` varchar(32) NOT NULL,
  `dmHost` varchar(150) DEFAULT NULL,
  `source_surl` varchar(900) DEFAULT NULL,
  `dest_surl` varchar(900) DEFAULT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `error_scope` varchar(32) DEFAULT NULL,
  `error_phase` varchar(32) DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  `checksum` varchar(100) DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `start_time` timestamp NULL DEFAULT NULL,
  `internal_file_params` varchar(255) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `pid` int(11) DEFAULT NULL,
  `tx_duration` double DEFAULT NULL,
  `retry` int(11) DEFAULT '0',
  `user_filesize` double DEFAULT NULL,
  `file_metadata` varchar(255) DEFAULT NULL,
  `activity` varchar(255) DEFAULT 'default',
  `selection_strategy` varchar(255) DEFAULT NULL,
  `dm_start` timestamp NULL DEFAULT NULL,
  `dm_finished` timestamp NULL DEFAULT NULL,
  `dm_token` varchar(255) DEFAULT NULL,
  `retry_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timeout` int(11) DEFAULT NULL,
  `hashed_id` int(10) unsigned DEFAULT '0',
  `vo_name` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`file_id`),
  KEY `dm_job_id` (`job_id`),
  CONSTRAINT `fk_job_id` FOREIGN KEY (`job_id`) REFERENCES `t_job` (`job_id`)
) ENGINE=InnoDB AUTO_INCREMENT=534226 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_dm_backup`
--

DROP TABLE IF EXISTS `t_dm_backup`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_dm_backup` (
  `file_id` int(11) NOT NULL DEFAULT '0',
  `job_id` char(36) NOT NULL,
  `file_state` varchar(32) NOT NULL,
  `dmHost` varchar(150) DEFAULT NULL,
  `source_surl` varchar(900) DEFAULT NULL,
  `dest_surl` varchar(900) DEFAULT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  `checksum` varchar(100) DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `start_time` timestamp NULL DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `tx_duration` double DEFAULT NULL,
  `retry` int(11) DEFAULT '0',
  `user_filesize` double DEFAULT NULL,
  `file_metadata` varchar(1024) DEFAULT NULL,
  `activity` varchar(255) DEFAULT 'default',
  `dm_token` varchar(255) DEFAULT NULL,
  `retry_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timeout` int(11) DEFAULT NULL,
  `hashed_id` int(10) unsigned DEFAULT '0',
  `vo_name` varchar(100) DEFAULT NULL
) ENGINE=ARCHIVE DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file`
--

DROP TABLE IF EXISTS `t_file`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_file` (
  `log_file_debug` tinyint(1) DEFAULT NULL,
  `file_id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `file_index` int(11) DEFAULT NULL,
  `job_id` char(36) NOT NULL,
  `file_state` enum('STAGING','STARTED','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','CANCELED','NOT_USED') NOT NULL,
  `transfer_host` varchar(255) DEFAULT NULL,
  `source_surl` varchar(1100) DEFAULT NULL,
  `dest_surl` varchar(1100) DEFAULT NULL,
  `source_se` varchar(255) DEFAULT NULL,
  `dest_se` varchar(255) DEFAULT NULL,
  `staging_host` varchar(1024) DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  `current_failures` int(11) DEFAULT NULL,
  `filesize` bigint(20) DEFAULT NULL,
  `checksum` varchar(100) DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `start_time` timestamp NULL DEFAULT NULL,
  `internal_file_params` varchar(255) DEFAULT NULL,
  `pid` int(11) DEFAULT NULL,
  `tx_duration` double DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `retry` int(11) DEFAULT '0',
  `user_filesize` bigint(20) DEFAULT NULL,
  `file_metadata` text,
  `selection_strategy` char(32) DEFAULT NULL,
  `staging_start` timestamp NULL DEFAULT NULL,
  `staging_finished` timestamp NULL DEFAULT NULL,
  `bringonline_token` varchar(255) DEFAULT NULL,
  `retry_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timeout` int(11) DEFAULT NULL,
  `log_file` varchar(2048) DEFAULT NULL,
  `t_log_file_debug` int(11) DEFAULT NULL,
  `hashed_id` int(10) unsigned DEFAULT '0',
  `vo_name` varchar(50) DEFAULT NULL,
  `activity` varchar(255) DEFAULT 'default',
  `transferred` bigint(20) DEFAULT '0',
  PRIMARY KEY (`file_id`),
  KEY `idx_job_id` (`job_id`),
  KEY `idx_waittimeout` (`wait_timeout`),
  KEY `idx_activity` (`vo_name`,`activity`),
  KEY `idx_link_state_vo` (`source_se`,`dest_se`,`file_state`,`vo_name`),
  KEY `idx_finish_time` (`finish_time`),
  KEY `idx_staging` (`file_state`,`vo_name`,`source_se`),
  KEY `idx_state_host` (`file_state`,`transfer_host`),
  CONSTRAINT `job_id` FOREIGN KEY (`job_id`) REFERENCES `t_job` (`job_id`)
) ENGINE=InnoDB AUTO_INCREMENT=4442981 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file_backup`
--

DROP TABLE IF EXISTS `t_file_backup`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_file_backup` (
  `file_id` int(11) NOT NULL DEFAULT '0',
  `file_index` int(11) DEFAULT NULL,
  `job_id` char(36) NOT NULL,
  `file_state` varchar(32) NOT NULL,
  `logical_name` varchar(1100) DEFAULT NULL,
  `symbolicName` varchar(255) DEFAULT NULL,
  `transferHost` varchar(150) DEFAULT NULL,
  `source_surl` varchar(900) DEFAULT NULL,
  `dest_surl` varchar(900) DEFAULT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `agent_dn` varchar(255) DEFAULT NULL,
  `error_scope` varchar(32) DEFAULT NULL,
  `error_phase` varchar(32) DEFAULT NULL,
  `reason_class` varchar(32) DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  `num_failures` int(11) DEFAULT NULL,
  `current_failures` int(11) DEFAULT NULL,
  `catalog_failures` int(11) DEFAULT NULL,
  `prestage_failures` int(11) DEFAULT NULL,
  `filesize` double DEFAULT NULL,
  `checksum` varchar(100) DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `start_time` timestamp NULL DEFAULT NULL,
  `internal_file_params` varchar(255) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `pid` int(11) DEFAULT NULL,
  `tx_duration` double DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `transferred` double DEFAULT '0',
  `retry` int(11) DEFAULT '0',
  `user_filesize` double DEFAULT NULL,
  `file_metadata` varchar(1024) DEFAULT NULL,
  `activity` varchar(255) DEFAULT 'default',
  `selection_strategy` varchar(255) DEFAULT NULL,
  `staging_start` timestamp NULL DEFAULT NULL,
  `staging_finished` timestamp NULL DEFAULT NULL,
  `bringonline_token` varchar(255) DEFAULT NULL,
  `retry_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timeout` int(11) DEFAULT NULL,
  `t_log_file` varchar(2048) DEFAULT NULL,
  `t_log_file_debug` int(11) DEFAULT NULL,
  `hashed_id` int(10) unsigned DEFAULT '0',
  `vo_name` varchar(100) DEFAULT NULL
) ENGINE=ARCHIVE DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file_old`
--

DROP TABLE IF EXISTS `t_file_old`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_file_old` (
  `file_id` int(11) NOT NULL AUTO_INCREMENT,
  `file_index` int(11) DEFAULT NULL,
  `job_id` char(36) NOT NULL,
  `file_state` varchar(32) NOT NULL,
  `logical_name` varchar(1100) DEFAULT NULL,
  `symbolicName` varchar(255) DEFAULT NULL,
  `transferHost` varchar(150) DEFAULT NULL,
  `source_surl` varchar(900) DEFAULT NULL,
  `dest_surl` varchar(900) DEFAULT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `agent_dn` varchar(255) DEFAULT NULL,
  `error_scope` varchar(32) DEFAULT NULL,
  `error_phase` varchar(32) DEFAULT NULL,
  `reason_class` varchar(32) DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  `num_failures` int(11) DEFAULT NULL,
  `current_failures` int(11) DEFAULT NULL,
  `catalog_failures` int(11) DEFAULT NULL,
  `prestage_failures` int(11) DEFAULT NULL,
  `filesize` double DEFAULT NULL,
  `checksum` varchar(100) DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `start_time` timestamp NULL DEFAULT NULL,
  `internal_file_params` varchar(255) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `pid` int(11) DEFAULT NULL,
  `tx_duration` double DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `retry` int(11) DEFAULT '0',
  `user_filesize` double DEFAULT NULL,
  `file_metadata` varchar(1024) DEFAULT NULL,
  `activity` varchar(255) DEFAULT 'default',
  `selection_strategy` varchar(255) DEFAULT NULL,
  `staging_start` timestamp NULL DEFAULT NULL,
  `staging_finished` timestamp NULL DEFAULT NULL,
  `bringonline_token` varchar(255) DEFAULT NULL,
  `retry_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timestamp` timestamp NULL DEFAULT NULL,
  `wait_timeout` int(11) DEFAULT NULL,
  `t_log_file` varchar(2048) DEFAULT NULL,
  `t_log_file_debug` int(11) DEFAULT NULL,
  `hashed_id` int(10) unsigned DEFAULT '0',
  `vo_name` varchar(100) DEFAULT NULL,
  `transferred` double DEFAULT '0',
  PRIMARY KEY (`file_id`),
  KEY `file_job_id` (`job_id`),
  KEY `file_jobfinished_id` (`job_finished`),
  KEY `file_source_dest` (`source_se`,`dest_se`,`file_state`),
  KEY `t_waittimeout` (`wait_timeout`),
  KEY `t_file_select` (`dest_se`,`source_se`,`job_finished`,`file_state`),
  KEY `file_vo_name_state` (`file_state`,`vo_name`,`source_se`,`dest_se`),
  KEY `file_tr_host` (`transferHost`,`file_state`),
  KEY `t_file_activity` (`activity`),
  CONSTRAINT `t_file_old_ibfk_1` FOREIGN KEY (`job_id`) REFERENCES `t_job_old` (`job_id`)
) ENGINE=InnoDB AUTO_INCREMENT=3770788 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file_retry_errors`
--

DROP TABLE IF EXISTS `t_file_retry_errors`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_file_retry_errors` (
  `file_id` bigint(20) unsigned NOT NULL,
  `attempt` int(11) NOT NULL,
  `datetime` timestamp NULL DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  PRIMARY KEY (`file_id`,`attempt`),
  CONSTRAINT `t_file_retry_errors_ibfk_1` FOREIGN KEY (`file_id`) REFERENCES `t_file` (`file_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file_retry_errors_old`
--

DROP TABLE IF EXISTS `t_file_retry_errors_old`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_file_retry_errors_old` (
  `file_id` int(11) NOT NULL,
  `attempt` int(11) NOT NULL,
  `datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `reason` varchar(2048) DEFAULT NULL,
  PRIMARY KEY (`file_id`,`attempt`),
  CONSTRAINT `t_file_retry_fk` FOREIGN KEY (`file_id`) REFERENCES `t_file_old` (`file_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file_share_config`
--

DROP TABLE IF EXISTS `t_file_share_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_file_share_config` (
  `file_id` bigint(20) unsigned NOT NULL,
  `source` varchar(150) NOT NULL,
  `destination` varchar(150) NOT NULL,
  `vo` varchar(100) NOT NULL,
  PRIMARY KEY (`file_id`,`source`,`destination`,`vo`),
  KEY `source` (`source`,`destination`,`vo`),
  CONSTRAINT `t_file_share_config_ibfk_1` FOREIGN KEY (`source`, `destination`, `vo`) REFERENCES `t_share_config` (`source`, `destination`, `vo`) ON DELETE CASCADE,
  CONSTRAINT `t_file_share_config_ibfk_2` FOREIGN KEY (`file_id`) REFERENCES `t_file` (`file_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file_share_config_old`
--

DROP TABLE IF EXISTS `t_file_share_config_old`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_file_share_config_old` (
  `file_id` int(11) NOT NULL,
  `source` varchar(150) NOT NULL,
  `destination` varchar(150) NOT NULL,
  `vo` varchar(100) NOT NULL,
  PRIMARY KEY (`file_id`,`source`,`destination`,`vo`),
  KEY `t_share_config_fk1` (`source`,`destination`,`vo`),
  CONSTRAINT `t_share_config_fk1` FOREIGN KEY (`source`, `destination`, `vo`) REFERENCES `t_share_config` (`source`, `destination`, `vo`) ON DELETE CASCADE,
  CONSTRAINT `t_share_config_fk2` FOREIGN KEY (`file_id`) REFERENCES `t_file_old` (`file_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_group_members`
--

DROP TABLE IF EXISTS `t_group_members`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_group_members` (
  `groupName` varchar(255) NOT NULL,
  `member` varchar(255) NOT NULL,
  PRIMARY KEY (`groupName`,`member`),
  UNIQUE KEY `member` (`member`),
  CONSTRAINT `t_group_members_fk` FOREIGN KEY (`member`) REFERENCES `t_se` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_hosts`
--

DROP TABLE IF EXISTS `t_hosts`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_hosts` (
  `hostname` varchar(64) NOT NULL,
  `beat` timestamp NULL DEFAULT NULL,
  `drain` int(11) DEFAULT '0',
  `service_name` varchar(64) NOT NULL,
  PRIMARY KEY (`hostname`,`service_name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_job`
--

DROP TABLE IF EXISTS `t_job`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_job` (
  `job_id` char(36) NOT NULL,
  `job_state` enum('STAGING','SUBMITTED','READY','ACTIVE','FINISHED','FAILED','FINISHEDDIRTY','CANCELED','DELETE') NOT NULL,
  `job_type` char(1) DEFAULT NULL,
  `cancel_job` char(1) DEFAULT NULL,
  `source_se` varchar(255) DEFAULT NULL,
  `dest_se` varchar(255) DEFAULT NULL,
  `user_dn` varchar(1024) DEFAULT NULL,
  `cred_id` char(16) DEFAULT NULL,
  `vo_name` varchar(50) DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  `submit_time` timestamp NULL DEFAULT NULL,
  `priority` int(11) DEFAULT '3',
  `submit_host` varchar(255) DEFAULT NULL,
  `max_time_in_queue` int(11) DEFAULT NULL,
  `space_token` varchar(255) DEFAULT NULL,
  `internal_job_params` varchar(255) DEFAULT NULL,
  `overwrite_flag` char(1) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `source_space_token` varchar(255) DEFAULT NULL,
  `copy_pin_lifetime` int(11) DEFAULT NULL,
  `checksum_method` char(1) DEFAULT NULL,
  `bring_online` int(11) DEFAULT NULL,
  `retry` int(11) DEFAULT '0',
  `retry_delay` int(11) DEFAULT '0',
  `job_metadata` text,
  PRIMARY KEY (`job_id`),
  KEY `idx_vo_name` (`vo_name`),
  KEY `idx_jobfinished` (`job_finished`),
  KEY `idx_link` (`source_se`,`dest_se`),
  KEY `idx_submission` (`submit_time`,`submit_host`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_job_backup`
--

DROP TABLE IF EXISTS `t_job_backup`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_job_backup` (
  `job_id` char(36) NOT NULL,
  `job_state` varchar(32) NOT NULL,
  `reuse_job` varchar(3) DEFAULT NULL,
  `cancel_job` char(1) DEFAULT NULL,
  `job_params` varchar(255) DEFAULT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `user_dn` varchar(255) NOT NULL,
  `agent_dn` varchar(255) DEFAULT NULL,
  `user_cred` varchar(255) DEFAULT NULL,
  `cred_id` varchar(100) DEFAULT NULL,
  `voms_cred` longtext,
  `vo_name` varchar(100) DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  `submit_time` timestamp NULL DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `priority` int(11) DEFAULT '3',
  `submit_host` varchar(150) DEFAULT NULL,
  `max_time_in_queue` int(11) DEFAULT NULL,
  `space_token` varchar(255) DEFAULT NULL,
  `storage_class` varchar(255) DEFAULT NULL,
  `myproxy_server` varchar(255) DEFAULT NULL,
  `internal_job_params` varchar(255) DEFAULT NULL,
  `overwrite_flag` char(1) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `source_space_token` varchar(255) DEFAULT NULL,
  `source_token_description` varchar(255) DEFAULT NULL,
  `copy_pin_lifetime` int(11) DEFAULT NULL,
  `fail_nearline` char(1) DEFAULT NULL,
  `checksum_method` varchar(10) DEFAULT NULL,
  `configuration_count` int(11) DEFAULT NULL,
  `bring_online` int(11) DEFAULT NULL,
  `retry` int(11) DEFAULT '0',
  `retry_delay` int(11) DEFAULT '0',
  `job_metadata` varchar(1024) DEFAULT NULL
) ENGINE=ARCHIVE DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_job_old`
--

DROP TABLE IF EXISTS `t_job_old`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_job_old` (
  `job_id` char(36) NOT NULL,
  `job_state` varchar(32) NOT NULL,
  `reuse_job` varchar(3) DEFAULT NULL,
  `cancel_job` char(1) DEFAULT NULL,
  `job_params` varchar(255) DEFAULT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `user_dn` varchar(255) NOT NULL,
  `agent_dn` varchar(255) DEFAULT NULL,
  `user_cred` varchar(255) DEFAULT NULL,
  `cred_id` varchar(100) DEFAULT NULL,
  `voms_cred` longtext,
  `vo_name` varchar(100) DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  `submit_time` timestamp NULL DEFAULT NULL,
  `finish_time` timestamp NULL DEFAULT NULL,
  `priority` int(11) DEFAULT '3',
  `submit_host` varchar(150) DEFAULT NULL,
  `max_time_in_queue` int(11) DEFAULT NULL,
  `space_token` varchar(255) DEFAULT NULL,
  `storage_class` varchar(255) DEFAULT NULL,
  `myproxy_server` varchar(255) DEFAULT NULL,
  `internal_job_params` varchar(255) DEFAULT NULL,
  `overwrite_flag` char(1) DEFAULT NULL,
  `job_finished` timestamp NULL DEFAULT NULL,
  `source_space_token` varchar(255) DEFAULT NULL,
  `source_token_description` varchar(255) DEFAULT NULL,
  `copy_pin_lifetime` int(11) DEFAULT NULL,
  `fail_nearline` char(1) DEFAULT NULL,
  `checksum_method` char(1) DEFAULT NULL,
  `configuration_count` int(11) DEFAULT NULL,
  `bring_online` int(11) DEFAULT NULL,
  `retry` int(11) DEFAULT '0',
  `retry_delay` int(11) DEFAULT '0',
  `job_metadata` varchar(1024) DEFAULT NULL,
  PRIMARY KEY (`job_id`),
  KEY `job_vo_name` (`vo_name`),
  KEY `job_jobfinished_id` (`job_finished`),
  KEY `t_job_source_se` (`source_se`),
  KEY `t_job_dest_se` (`dest_se`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_link_config`
--

DROP TABLE IF EXISTS `t_link_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_link_config` (
  `source` varchar(150) NOT NULL,
  `destination` varchar(150) NOT NULL,
  `state` varchar(30) NOT NULL,
  `symbolicName` varchar(255) NOT NULL,
  `nostreams` int(11) NOT NULL,
  `tcp_buffer_size` int(11) DEFAULT '0',
  `urlcopy_tx_to` int(11) NOT NULL,
  `auto_tuning` varchar(3) DEFAULT NULL,
  PRIMARY KEY (`source`,`destination`),
  UNIQUE KEY `symbolicName` (`symbolicName`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_oauth2_apps`
--

DROP TABLE IF EXISTS `t_oauth2_apps`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
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
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_oauth2_codes` (
  `client_id` varchar(64) DEFAULT NULL,
  `code` varchar(128) NOT NULL,
  `scope` varchar(512) DEFAULT NULL,
  `dlg_id` varchar(100) NOT NULL,
  PRIMARY KEY (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_oauth2_tokens`
--

DROP TABLE IF EXISTS `t_oauth2_tokens`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
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
-- Table structure for table `t_optimize`
--

DROP TABLE IF EXISTS `t_optimize`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_optimize` (
  `auto_number` int(11) NOT NULL AUTO_INCREMENT,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `nostreams` int(11) DEFAULT NULL,
  `active` int(11) DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `udt` varchar(3) DEFAULT NULL,
  `ipv6` varchar(3) DEFAULT NULL,
  PRIMARY KEY (`auto_number`),
  KEY `optimize_source_a` (`source_se`,`dest_se`)
) ENGINE=InnoDB AUTO_INCREMENT=47 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_optimize_active`
--

DROP TABLE IF EXISTS `t_optimize_active`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_optimize_active` (
  `source_se` varchar(150) NOT NULL,
  `dest_se` varchar(150) NOT NULL,
  `active` int(10) unsigned DEFAULT '5',
  `datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `ema` double DEFAULT '0',
  `fixed` varchar(3) DEFAULT NULL,
  `min_active` int(11) DEFAULT NULL,
  `max_active` int(11) DEFAULT NULL,
  PRIMARY KEY (`source_se`,`dest_se`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_optimize_mode`
--

DROP TABLE IF EXISTS `t_optimize_mode`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_optimize_mode` (
  `mode_opt` int(11) NOT NULL DEFAULT '1'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_optimize_streams`
--

DROP TABLE IF EXISTS `t_optimize_streams`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_optimize_streams` (
  `source_se` varchar(150) NOT NULL,
  `dest_se` varchar(150) NOT NULL,
  `nostreams` int(11) NOT NULL,
  `datetime` timestamp NULL DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `tested` int(11) DEFAULT '0',
  PRIMARY KEY (`source_se`,`dest_se`,`nostreams`),
  KEY `t_optimize_streams_datetime` (`datetime`),
  KEY `t_optimize_streams_throughput` (`throughput`),
  CONSTRAINT `t_optimize_streams_fk` FOREIGN KEY (`source_se`, `dest_se`) REFERENCES `t_optimize_active` (`source_se`, `dest_se`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_optimizer_evolution`
--

DROP TABLE IF EXISTS `t_optimizer_evolution`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_optimizer_evolution` (
  `datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `active` int(11) DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `success` float DEFAULT NULL,
  `rationale` text,
  `diff` int(11) DEFAULT '0',
  `actual_active` int(11) DEFAULT NULL,
  `queue_size` int(11) DEFAULT NULL,
  `ema` double DEFAULT NULL,
  `filesize_avg` double DEFAULT NULL,
  `filesize_stddev` double DEFAULT NULL,
  KEY `t_optimizer_source_and_dest` (`source_se`,`dest_se`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_profiling_info`
--

DROP TABLE IF EXISTS `t_profiling_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_profiling_info` (
  `period` int(11) NOT NULL,
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_profiling_snapshot`
--

DROP TABLE IF EXISTS `t_profiling_snapshot`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_profiling_snapshot` (
  `scope` varchar(255) NOT NULL,
  `cnt` int(11) NOT NULL,
  `exceptions` int(11) NOT NULL,
  `total` double NOT NULL,
  `average` double NOT NULL,
  PRIMARY KEY (`scope`),
  KEY `t_prof_snapshot_total` (`total`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_schema_vers`
--

DROP TABLE IF EXISTS `t_schema_vers`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_schema_vers` (
  `major` int(11) NOT NULL,
  `minor` int(11) NOT NULL,
  `patch` int(11) NOT NULL,
  `message` text
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (3, 0, 0, 'Schema 3.0.0');

--
-- Table structure for table `t_se`
--

DROP TABLE IF EXISTS `t_se`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_se` (
  `se_id_info` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `endpoint` varchar(1024) DEFAULT NULL,
  `se_type` varchar(30) DEFAULT NULL,
  `site` varchar(100) DEFAULT NULL,
  `state` varchar(30) DEFAULT NULL,
  `version` varchar(30) DEFAULT NULL,
  `host` varchar(100) DEFAULT NULL,
  `se_transfer_type` varchar(30) DEFAULT NULL,
  `se_transfer_protocol` varchar(30) DEFAULT NULL,
  `se_control_protocol` varchar(30) DEFAULT NULL,
  `gocdb_id` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`name`),
  KEY `se_id_info` (`se_id_info`)
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_server_config`
--

DROP TABLE IF EXISTS `t_server_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_server_config` (
  `retry` int(11) DEFAULT '0',
  `max_time_queue` int(11) DEFAULT '0',
  `sec_per_mb` int(11) DEFAULT '0',
  `global_timeout` int(11) DEFAULT '0',
  `vo_name` varchar(100) DEFAULT NULL,
  `show_user_dn` varchar(3) DEFAULT NULL,
  `max_per_link` int(11) DEFAULT '0',
  `max_per_se` int(11) DEFAULT '0',
  `global_tcp_stream` int(11) DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_share_config`
--

DROP TABLE IF EXISTS `t_share_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_share_config` (
  `source` varchar(150) NOT NULL,
  `destination` varchar(150) NOT NULL,
  `vo` varchar(100) NOT NULL,
  `active` int(11) NOT NULL,
  PRIMARY KEY (`source`,`destination`,`vo`),
  CONSTRAINT `t_share_config_fk` FOREIGN KEY (`source`, `destination`) REFERENCES `t_link_config` (`source`, `destination`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_stage_req`
--

DROP TABLE IF EXISTS `t_stage_req`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_stage_req` (
  `vo_name` varchar(100) NOT NULL,
  `host` varchar(150) NOT NULL,
  `operation` varchar(150) NOT NULL,
  `concurrent_ops` int(11) DEFAULT '0',
  PRIMARY KEY (`vo_name`,`host`,`operation`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_vo_acl`
--

DROP TABLE IF EXISTS `t_vo_acl`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_vo_acl` (
  `vo_name` varchar(50) NOT NULL,
  `principal` varchar(255) NOT NULL,
  PRIMARY KEY (`vo_name`,`principal`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Temporary table structure for view `v_staging`
--

DROP TABLE IF EXISTS `v_staging`;
/*!50001 DROP VIEW IF EXISTS `v_staging`*/;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
/*!50001 CREATE TABLE `v_staging` (
 `job_id` tinyint NOT NULL,
  `file_id` tinyint NOT NULL,
  `hashed_id` tinyint NOT NULL,
  `vo_name` tinyint NOT NULL,
  `source_se` tinyint NOT NULL,
  `file_state` tinyint NOT NULL,
  `source_surl` tinyint NOT NULL
) ENGINE=MyISAM */;
SET character_set_client = @saved_cs_client;

--
-- Temporary table structure for view `vw_staging_not_started`
--

DROP TABLE IF EXISTS `vw_staging_not_started`;
/*!50001 DROP VIEW IF EXISTS `vw_staging_not_started`*/;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
/*!50001 CREATE TABLE `vw_staging_not_started` (
 `file_state` tinyint NOT NULL,
  `job_id` tinyint NOT NULL,
  `file_id` tinyint NOT NULL,
  `vo_name` tinyint NOT NULL,
  `source_se` tinyint NOT NULL,
  `source_surl` tinyint NOT NULL,
  `hashed_id` tinyint NOT NULL
) ENGINE=MyISAM */;
SET character_set_client = @saved_cs_client;

--
-- Final view structure for view `v_staging`
--

/*!50001 DROP TABLE IF EXISTS `v_staging`*/;
/*!50001 DROP VIEW IF EXISTS `v_staging`*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = latin1 */;
/*!50001 SET character_set_results     = latin1 */;
/*!50001 SET collation_connection      = latin1_swedish_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED */
/*!50013 DEFINER=`fts3`@`%` SQL SECURITY DEFINER */
/*!50001 VIEW `v_staging` AS select `q`.`job_id` AS `job_id`,`q`.`file_id` AS `file_id`,`q`.`hashed_id` AS `hashed_id`,`q`.`vo_name` AS `vo_name`,`q`.`source_se` AS `source_se`,`q`.`file_state` AS `file_state`,`q`.`source_surl` AS `source_surl` from (`t_file` `q` left join `t_file` `s` on(((`q`.`source_surl` = `s`.`source_surl`) and (`q`.`vo_name` = `s`.`vo_name`) and (`s`.`source_se` = `q`.`source_se`) and (`s`.`file_state` = 'STARTED')))) where ((`q`.`file_state` = 'STAGING') and isnull(`s`.`file_state`)) */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;

--
-- Final view structure for view `vw_staging_not_started`
--

/*!50001 DROP TABLE IF EXISTS `vw_staging_not_started`*/;
/*!50001 DROP VIEW IF EXISTS `vw_staging_not_started`*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = latin1 */;
/*!50001 SET character_set_results     = latin1 */;
/*!50001 SET collation_connection      = latin1_swedish_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED */
/*!50013 DEFINER=`fts3`@`%` SQL SECURITY DEFINER */
/*!50001 VIEW `vw_staging_not_started` AS (select `q`.`file_state` AS `file_state`,`q`.`job_id` AS `job_id`,`q`.`file_id` AS `file_id`,`q`.`vo_name` AS `vo_name`,`q`.`source_se` AS `source_se`,`q`.`source_surl` AS `source_surl`,`q`.`hashed_id` AS `hashed_id` from (`t_file` `q` left join `t_file` `s` on(((`q`.`source_surl` = `s`.`source_surl`) and (`q`.`vo_name` = `s`.`vo_name`) and (`s`.`source_se` = `q`.`source_se`)))) where ((`q`.`file_state` = 'STAGING') and (`s`.`file_state` = 'STARTED'))) */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2017-01-23  9:44:58
