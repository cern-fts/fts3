--
-- FTS3 Schema 10.0.0
-- [FTS-1782] Netlink Throughput Limitation
--

ALTER TABLE `t_optimizer`
    ADD COLUMN `actual_active` int(11) DEFAULT NULL,
    ADD COLUMN `throughput` double DEFAULT '0',
    ADD COLUMN `queue_size` int(11) DEFAULT NULL;

--
-- Table structure for table `t_netlink_stat`
--

CREATE TABLE IF NOT EXISTS `t_netlink_stat` (
  `netlink_id` char(36) NOT NULL,
  `head_ip` varchar(150) DEFAULT '*',
  `tail_ip` varchar(150) DEFAULT '*',
  `head_asn` int(32) DEFAULT '0',
  `tail_asn` int(32) DEFAULT '0',
  `head_rdns` varchar(253) DEFAULT NULL,
  `tail_rdns` varchar(253) DEFAULT NULL,
  `capacity` float DEFAULT NULL,
  PRIMARY KEY (`netlink_id`),
  CONSTRAINT `idx_ports` UNIQUE (`head_ip`, `tail_ip`)
);

--
-- Table structure for table `t_netlink_trace`
--

CREATE TABLE IF NOT EXISTS `t_netlink_trace` (
  `trace_id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `source_se` varchar(255) NOT NULL,
  `dest_se` varchar(255) NOT NULL,
  `hop_idx` int(8) NOT NULL,
  `netlink` char(36) NOT NULL,
  PRIMARY KEY (`trace_id`),
  CONSTRAINT `idx_pair_hop` UNIQUE (`source_se`, `dest_se`, `hop_idx`),
  CONSTRAINT `fk_netlink` FOREIGN KEY (`netlink`) REFERENCES `t_netlink_stat` (`netlink_id`)
);

--
-- Table structure for table `t_netlink_config`
--

CREATE TABLE IF NOT EXISTS `t_netlink_config` (
  `head_ip` varchar(150) NOT NULL,
  `tail_ip` varchar(150) NOT NULL,
  `netlink_name` varchar(150) NOT NULL,
  `min_active` int(11) DEFAULT NULL,
  `max_active` int(11) DEFAULT NULL,
  `max_throughput` float DEFAULT NULL,
  PRIMARY KEY (`head_ip`,`tail_ip`),
  UNIQUE KEY `netlink_name` (`netlink_name`)
);

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (10, 0, 0, 'FTS-1782: SE Throughput Limitation');
