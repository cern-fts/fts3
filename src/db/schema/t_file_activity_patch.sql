-- add activity column to t_file table
ALTER TABLE t_file
ADD activity   VARCHAR(255) DEFAULT "default";

-- create the index on activity
CREATE INDEX t_file_activity ON t_file(activity);