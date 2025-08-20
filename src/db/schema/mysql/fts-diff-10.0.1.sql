--
-- FTS3 Schema 10.0.0
-- [FTS-FTS-2259] Add an index on the t_file table to optimize queries filtering by source_se, dest_se, file_state, and finish_time
--

CREATE INDEX idx_link_state_finish_time ON t_file (source_se, dest_se, file_state, finish_time);