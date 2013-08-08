CREATE TABLE t_profiling_info (
    period  INT NOT NULL,
    updated TIMESTAMP NOT NULL
);

CREATE TABLE t_profiling_snapshot (
    scope      VARCHAR(255) NOT NULL PRIMARY KEY,
    cnt        INT NOT NULL,
    exceptions INT NOT NULL,
    total      NUMBER NOT NULL,
    average    NUMBER NOT NULL
);

CREATE INDEX t_prof_snapshot_total ON t_profiling_snapshot(total);


