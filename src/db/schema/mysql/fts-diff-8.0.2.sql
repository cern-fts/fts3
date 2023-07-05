--
-- FTS3 Schema 8.0.2
-- [FTS-1914] Schema changes TPC support configuration
--

ALTER TABLE `t_se`
    ADD COLUMN `tpc_support` VARCHAR(10) DEFAULT NULL;
