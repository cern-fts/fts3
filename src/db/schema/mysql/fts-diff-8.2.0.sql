--
-- FTS3 Schema 8.2.0
-- [FTS-1782] SE Throughput Limitation
--

DROP TABLE IF EXISTS `t_optimizer`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `t_optimizer` (
  `source_se` varchar(150) NOT NULL,
  `dest_se` varchar(150) NOT NULL,
  `datetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `ema` double DEFAULT '0',
  `active` int(11) DEFAULT '2',
  `nostreams` int(11) DEFAULT '1',
  `actual_active` int(11) DEFAULT NULL,
  `throughput` double DEFAULT '0',
  `queue_size` int(11) DEFAULT NULL,
  PRIMARY KEY (`source_se`,`dest_se`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (8, 2, 0, 'FTS-1782: SE Throughput Limitation');