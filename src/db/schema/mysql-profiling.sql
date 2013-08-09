CREATE TABLE t_profiling_info (
    period  INT NOT NULL,
    updated TIMESTAMP NOT NULL
);

CREATE TABLE t_profiling_snapshot (
    scope      VARCHAR(255) NOT NULL PRIMARY KEY,
    cnt        LONG NOT NULL,
    exceptions LONG NOT NULL,
    total      DOUBLE NOT NULL,
    average    DOUBLE NOT NULL
);


CREATE TABLE t_optimize_mode (
  mode       INTEGER NOT NULL DEFAULT 1
);

CREATE INDEX t_prof_snapshot_total ON t_profiling_snapshot(total);

