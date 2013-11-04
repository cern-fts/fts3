-- add activity column to t_file table
ALTER TABLE t_file
ADD activity   VARCHAR(255) DEFAULT "default";


ALTER TABLE t_optimize_active
ADD message   VARCHAR(512);

ALTER TABLE t_optimize_active
ADD datetime   TIMESTAMP;

-- create the index on activity
CREATE INDEX t_file_activity ON t_file(activity);
