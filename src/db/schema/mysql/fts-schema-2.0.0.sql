-- MySQL dump 10.13  Distrib 5.6.31, for Linux (x86_64)
--
-- Host: arioch    Database: fts3
-- ------------------------------------------------------
-- Server version	5.1.73

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
  `activity_share` varchar(255) NOT NULL,
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
  `se` varchar(150) NOT NULL DEFAULT '',
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
  `cloudStorage_name` varchar(128) NOT NULL,
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
  `cloudStorage_name` varchar(128) NOT NULL,
  `access_token` varchar(255) DEFAULT NULL,
  `access_token_secret` varchar(255) DEFAULT NULL,
  `request_token` varchar(512) DEFAULT NULL,
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
  `datetime` timestamp NULL DEFAULT NULL,
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
-- Table structure for table `t_credential_cache`
--

DROP TABLE IF EXISTS `t_credential_cache`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_credential_cache` (
  `dlg_id` varchar(100) NOT NULL DEFAULT '',
  `dn` varchar(255) NOT NULL DEFAULT '',
  `cert_request` longtext,
  `priv_key` longtext,
  `voms_attrs` longtext,
  PRIMARY KEY (`dlg_id`,`dn`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_credential_vers`
--

DROP TABLE IF EXISTS `t_credential_vers`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_credential_vers` (
  `major` int(11) NOT NULL,
  `minor` int(11) NOT NULL,
  `patch` int(11) NOT NULL
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
  `debug` varchar(3) DEFAULT NULL,
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
  `vo_name` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`file_id`),
  KEY `t_dm_job_id` (`job_id`),
  KEY `t_dm_all` (`vo_name`,`source_se`,`file_state`),
  KEY `t_dm_source` (`source_se`,`file_state`),
  KEY `t_dm_state` (`file_state`,`hashed_id`),
  CONSTRAINT `t_dm_ibfk_1` FOREIGN KEY (`job_id`) REFERENCES `t_job` (`job_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
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
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file`
--

DROP TABLE IF EXISTS `t_file`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_file` (
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
  `vo_name` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`file_id`),
  KEY `file_job_id` (`job_id`),
  KEY `file_jobfinished_id` (`job_finished`),
  KEY `file_source_dest` (`source_se`,`dest_se`,`file_state`),
  KEY `t_waittimeout` (`wait_timeout`),
  KEY `t_file_select` (`dest_se`,`source_se`,`job_finished`,`file_state`),
  KEY `file_vo_name_state` (`file_state`,`vo_name`,`source_se`,`dest_se`),
  KEY `file_tr_host` (`transferHost`,`file_state`),
  KEY `t_file_activity` (`activity`),
  CONSTRAINT `t_file_ibfk_1` FOREIGN KEY (`job_id`) REFERENCES `t_job` (`job_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
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
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file_retry_errors`
--

DROP TABLE IF EXISTS `t_file_retry_errors`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_file_retry_errors` (
  `file_id` int(11) NOT NULL,
  `attempt` int(11) NOT NULL,
  `datetime` timestamp NULL DEFAULT NULL,
  `reason` varchar(2048) DEFAULT NULL,
  PRIMARY KEY (`file_id`,`attempt`),
  CONSTRAINT `t_file_retry_fk` FOREIGN KEY (`file_id`) REFERENCES `t_file` (`file_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_file_share_config`
--

DROP TABLE IF EXISTS `t_file_share_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_file_share_config` (
  `file_id` int(11) NOT NULL,
  `source` varchar(150) NOT NULL,
  `destination` varchar(150) NOT NULL,
  `vo` varchar(100) NOT NULL,
  PRIMARY KEY (`file_id`,`source`,`destination`,`vo`),
  KEY `t_share_config_fk1` (`source`,`destination`,`vo`),
  CONSTRAINT `t_share_config_fk1` FOREIGN KEY (`source`, `destination`, `vo`) REFERENCES `t_share_config` (`source`, `destination`, `vo`) ON DELETE CASCADE,
  CONSTRAINT `t_share_config_fk2` FOREIGN KEY (`file_id`) REFERENCES `t_file` (`file_id`) ON DELETE CASCADE
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
  `member` varchar(150) NOT NULL,
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
  `service_name` varchar(64) NOT NULL,
  `beat` timestamp NULL DEFAULT NULL,
  `drain` int(11) DEFAULT '0',
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
  `job_metadata` varchar(1024) DEFAULT NULL,
  PRIMARY KEY (`job_id`),
  KEY `job_vo_name` (`vo_name`),
  KEY `job_jobfinished_id` (`job_finished`),
  KEY `t_job_source_se` (`source_se`),
  KEY `t_job_dest_se` (`dest_se`)
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
  `job_metadata` varchar(1024) DEFAULT NULL,
  KEY `t_job_backup_job_id` (`job_id`)
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
  `no_tx_activity_to` int(11) DEFAULT '360',
  `auto_tuning` varchar(3) DEFAULT NULL,
  `placeholder1` int(11) DEFAULT NULL,
  `placeholder2` int(11) DEFAULT NULL,
  `placeholder3` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`source`,`destination`),
  UNIQUE KEY `symbolicName` (`symbolicName`)
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
  `file_id` int(11) NOT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `nostreams` int(11) DEFAULT NULL,
  `timeout` int(11) DEFAULT NULL,
  `active` int(11) DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `buffer` int(11) DEFAULT NULL,
  `filesize` double DEFAULT NULL,
  `datetime` timestamp NULL DEFAULT NULL,
  `udt` varchar(3) DEFAULT NULL,
  `ipv6` varchar(3) DEFAULT NULL,
  PRIMARY KEY (`auto_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
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
  `active` int(10) unsigned DEFAULT '2',
  `datetime` timestamp NULL DEFAULT NULL,
  `ema` double DEFAULT '0',
  `fixed` varchar(3) DEFAULT NULL,
  `min_active` int(11) DEFAULT NULL,
  `max_active` int(11) DEFAULT NULL,
  PRIMARY KEY (`source_se`,`dest_se`),
  KEY `t_optimize_active_datetime` (`datetime`)
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
  PRIMARY KEY (`source_se`,`dest_se`),
  KEY `t_optimize_streams_datetime` (`datetime`),
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
  `datetime` timestamp NULL DEFAULT NULL,
  `source_se` varchar(150) DEFAULT NULL,
  `dest_se` varchar(150) DEFAULT NULL,
  `active` int(11) DEFAULT NULL,
  `throughput` float DEFAULT NULL,
  `success` float DEFAULT NULL,
  `ema` float DEFAULT NULL,
  `rationale` text,
  `diff` int(11) DEFAULT '0',
  `actual_active` int(11) DEFAULT NULL,
  `queue_size` int(11) DEFAULT NULL,
  `filesize_avg` double DEFAULT NULL,
  `filesize_stddev` double DEFAULT NULL,
  KEY `t_optimizer_source_and_dest` (`source_se`,`dest_se`),
  KEY `t_optimizer_evolution_datetime` (`datetime`)
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

INSERT INTO `t_schema_vers` VALUES (2,0,0,'Schema 2.0.0');

--
-- Table structure for table `t_se`
--

DROP TABLE IF EXISTS `t_se`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_se` (
  `se_id_info` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(150) NOT NULL,
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
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_se_acl`
--

DROP TABLE IF EXISTS `t_se_acl`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_se_acl` (
  `name` varchar(150) NOT NULL DEFAULT '',
  `vo` varchar(100) NOT NULL DEFAULT '',
  PRIMARY KEY (`name`,`vo`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_se_pair_acl`
--

DROP TABLE IF EXISTS `t_se_pair_acl`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_se_pair_acl` (
  `se_pair_name` varchar(32) NOT NULL DEFAULT '',
  `principal` varchar(255) NOT NULL,
  PRIMARY KEY (`se_pair_name`,`principal`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
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
  `global_timeout` int(11) DEFAULT '0',
  `sec_per_mb` int(11) DEFAULT '0',
  `vo_name` varchar(100) DEFAULT NULL,
  `show_user_dn` varchar(3) DEFAULT NULL,
  `max_per_se` int(11) DEFAULT '0',
  `max_per_link` int(11) DEFAULT '0',
  `global_tcp_stream` int(11) DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `t_server_sanity`
--

DROP TABLE IF EXISTS `t_server_sanity`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_server_sanity` (
  `revertToSubmitted` tinyint(1) DEFAULT '0',
  `cancelWaitingFiles` tinyint(1) DEFAULT '0',
  `revertNotUsedFiles` tinyint(1) DEFAULT '0',
  `forceFailTransfers` tinyint(1) DEFAULT '0',
  `setToFailOldQueuedJobs` tinyint(1) DEFAULT '0',
  `checkSanityState` tinyint(1) DEFAULT '0',
  `cleanUpRecords` tinyint(1) DEFAULT '0',
  `msgcron` tinyint(1) DEFAULT '0',
  `t_revertToSubmitted` timestamp NULL DEFAULT NULL,
  `t_cancelWaitingFiles` timestamp NULL DEFAULT NULL,
  `t_revertNotUsedFiles` timestamp NULL DEFAULT NULL,
  `t_forceFailTransfers` timestamp NULL DEFAULT NULL,
  `t_setToFailOldQueuedJobs` timestamp NULL DEFAULT NULL,
  `t_checkSanityState` timestamp NULL DEFAULT NULL,
  `t_cleanUpRecords` timestamp NULL DEFAULT NULL,
  `t_msgcron` timestamp NULL DEFAULT NULL
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
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2016-07-14 11:23:19
