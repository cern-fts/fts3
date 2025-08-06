--
-- Script to downgrade from FTS3 Schema 10.0.1 to the previous schema (10.0.0)
--

DROP INDEX idx_link_state_finish_time ON t_file;
