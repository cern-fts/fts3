--
-- FTS3 Schema 10.0.1
-- [FTS-2259] Index to optimize the listing of failed/finished jobs in the WebUI
--

CREATE INDEX idx_link_state_finish_time ON t_file (source_se, dest_se, file_state, finish_time);
