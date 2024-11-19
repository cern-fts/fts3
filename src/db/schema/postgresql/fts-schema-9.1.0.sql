CREATE OR REPLACE FUNCTION update_datetime() RETURNS trigger AS $$
BEGIN
    NEW.datetime = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TABLE t_token_provider (
    name          VARCHAR(255)  NOT NULL,
    issuer        VARCHAR(1024) NOT NULL,
    client_id     VARCHAR(255)  NOT NULL,
    client_secret VARCHAR(255)  NOT NULL,
    PRIMARY KEY (issuer)
);

CREATE TABLE t_token (
    token_id                   VARCHAR(16)   NOT NULL,
    access_token               TEXT          NOT NULL,
    access_token_not_before    TIMESTAMP     NOT NULL,
    access_token_expiry        TIMESTAMP     NOT NULL,
    access_token_refresh_after TIMESTAMP     NOT NULL,
    refresh_token              TEXT              NULL,
    issuer                     VARCHAR(1024) NOT NULL,
    scope                      VARCHAR(1024) NOT NULL,
    audience                   VARCHAR(1024) NOT NULL,
    retry_timestamp            TIMESTAMP         NULL,
    retry_delay_m              INTEGER           NULL DEFAULT 0,
    attempts                   INTEGER           NULL DEFAULT 0,
    exchange_message           VARCHAR(2048)     NULL,
    retired                    SMALLINT      NOT NULL DEFAULT 0,
    PRIMARY KEY (token_id),
    CONSTRAINT fk_token_issuer
        FOREIGN KEY (issuer)
        REFERENCES t_token_provider(issuer)
);


CREATE TABLE t_gridmap (
  dn VARCHAR(255) NOT NULL,
  vo VARCHAR(100) NOT NULL,
  PRIMARY KEY (dn)
);

CREATE TABLE t_credential (
  dlg_id           VARCHAR(16)  NOT NULL,
  dn               VARCHAR(255) NOT NULL,
  proxy            TEXT             NULL,
  voms_attrs       TEXT             NULL,
  termination_time TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (dlg_id, dn)
);
CREATE INDEX idx_credential_termination_time ON t_credential(termination_time);

CREATE TABLE t_credential_cache (
  dlg_id       VARCHAR(16)  NOT NULL,
  dn           VARCHAR(255) NOT NULL,
  cert_request TEXT             NULL,
  priv_key     TEXT             NULL,
  voms_attrs   TEXT             NULL,
  PRIMARY KEY (dlg_id, dn)
);

CREATE TYPE enum_job_state AS ENUM(
    'STAGING',
    'ARCHIVING',
    'QOS_TRANSITION',
    'QOS_REQUEST_SUBMITTED',
    'SUBMITTED',
    'READY',
    'ACTIVE',
    'FINISHED',
    'FAILED',
    'FINISHEDDIRTY',
    'CANCELED',
    'DELETE'
);
CREATE TABLE t_job (
    job_id              VARCHAR(36)    NOT NULL,
    job_state           enum_job_state NOT NULL,
    job_type            VARCHAR(1)     NOT NULL,
    cancel_job          VARCHAR(1)         NULL,
    source_se           VARCHAR(255)       NULL,
    dest_se             VARCHAR(255)       NULL,
    user_dn             VARCHAR(1024)      NULL,
    cred_id             VARCHAR(16)        NULL,
    vo_name             VARCHAR(50)        NULL,
    reason              VARCHAR(2048)      NULL,
    submit_time         TIMESTAMP          NULL,
    priority            INTEGER        NOT NULL DEFAULT 3,
    submit_host         VARCHAR(255)   NOT NULL,
    max_time_in_queue   INTEGER            NULL,
    space_token         VARCHAR(255)       NULL,
    internal_job_params VARCHAR(255)       NULL,
    overwrite_flag      VARCHAR(1)         NULL,
    job_finished        TIMESTAMP          NULL,
    source_space_token  VARCHAR(255)       NULL,
    copy_pin_lifetime   INTEGER            NULL,
    checksum_method     VARCHAR(1)         NULL,
    bring_online        INTEGER            NULL,
    retry               INTEGER        NOT NULL DEFAULT 0,
    retry_delay         INTEGER        NOT NULL DEFAULT 0,
    target_qos          VARCHAR(255)       NULL,
    job_metadata        TEXT               NULL,
    archive_timeout     INTEGER            NULL,
    dst_file_report     VARCHAR(1)         NULL,
    os_project_id       VARCHAR(512)       NULL,
    PRIMARY KEY (job_id)
);
CREATE INDEX idx_job_vo_name     ON t_job(vo_name);
CREATE INDEX idx_job_jobfinished ON t_job(job_finished);
CREATE INDEX idx_job_link        ON t_job(source_se, dest_se);
CREATE INDEX idx_job_submission  ON t_job(submit_time, submit_host);
CREATE INDEX idx_job_job_type    ON t_job(job_type);

CREATE TABLE t_job_backup (
  job_id                VARCHAR(36)    NOT NULL,
  job_state             enum_job_state NOT NULL,
  job_type              VARCHAR(1)         NULL,
  cancel_job            VARCHAR(1)         NULL,
  source_se             VARCHAR(255)       NULL,
  dest_se               VARCHAR(255)       NULL,
  user_dn               VARCHAR(1024)      NULL,
  cred_id               VARCHAR(16)        NULL,
  vo_name               VARCHAR(50)        NULL,
  reason                VARCHAR(2048)      NULL,
  submit_time           TIMESTAMP          NULL,
  priority              INTEGER        NOT NULL DEFAULT 3,
  submit_host           VARCHAR(255)       NULL,
  max_time_in_queue     INTEGER            NULL,
  space_token           VARCHAR(255)       NULL,
  internal_job_params   VARCHAR(255)       NULL,
  overwrite_flag        VARCHAR(1)         NULL,
  job_finished          TIMESTAMP          NULL,
  source_space_token    VARCHAR(255)       NULL,
  copy_pin_lifetime     INTEGER            NULL,
  checksum_method       VARCHAR(1)         NULL,
  bring_online          INTEGER            NULL,
  retry                 INTEGER        NOT NULL DEFAULT 0,
  retry_delay           INTEGER        NOT NULL DEFAULT 0,
  target_qos            VARCHAR(255)       NULL,
  job_metadata          TEXT               NULL,
  archive_timeout       INTEGER            NULL,
  dst_file_report       VARCHAR(1)         NULL,
  os_project_id         VARCHAR(512)       NULL
);

CREATE TYPE enum_file_state AS ENUM(
    'STAGING',
    'ARCHIVING',
    'QOS_TRANSITION',
    'QOS_REQUEST_SUBMITTED',
    'STARTED',
    'SELECTED',
    'SUBMITTED',
    'READY',
    'ACTIVE',
    'REPLICA_FAILED',
    'HOP_FINISHED',
    'FINISHED',
    'FAILED',
    'CANCELED',
    'NOT_USED',
    'ON_HOLD',
    'ON_HOLD_STAGING',
    'FORCE_START',
    'TOKEN_PREP',
    'SCHEDULED'
);

CREATE TABLE t_queue (
    queue_id      BIGSERIAL       NOT NULL,
    created       TIMESTAMP       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    vo_name       VARCHAR(50)     NOT NULL,
    source_se     VARCHAR(255)    NOT NULL,
    dest_se       VARCHAR(255)    NOT NULL,
    activity      VARCHAR(255)    NOT NULL,
    file_state    enum_file_state NOT NULL,
    nb_files      BIGINT          NOT NULL DEFAULT 0,
    PRIMARY KEY (queue_id),
    UNIQUE (vo_name, source_se, dest_se, activity, file_state),
    CHECK (vo_name   != ''),
    CHECK (source_se != ''),
    CHECK (dest_se   != ''),
    CHECK (activity  != ''),
    CHECK (nb_files  >=  0)
);

CREATE TABLE t_file (
    log_file_debug       SMALLINT             NULL,
    file_id              BIGSERIAL        NOT NULL,
    file_index           INTEGER          NOT NULL,
    job_id               VARCHAR(36)      NOT NULL,
    file_state           enum_file_state  NOT NULL,
    file_state_initial   VARCHAR(32)          NULL,
    transfer_host        VARCHAR(255)         NULL,
    source_surl          VARCHAR(1100)    NOT NULL,
    dest_surl            VARCHAR(1100)    NOT NULL,
    source_se            VARCHAR(255)     NOT NULL,
    dest_se              VARCHAR(255)     NOT NULL,
    staging_host         VARCHAR(1024)        NULL,
    reason               VARCHAR(2048)        NULL,
    current_failures     INTEGER              NULL,
    filesize             BIGINT               NULL,
    checksum             VARCHAR(100)         NULL,
    finish_time          TIMESTAMP            NULL,
    start_time           TIMESTAMP            NULL,
    internal_file_params VARCHAR(255)         NULL,
    pid                  INTEGER              NULL,
    tx_duration          DOUBLE PRECISION     NULL,
    throughput           REAL                 NULL,
    retry                INTEGER              NULL DEFAULT 0,
    user_filesize        BIGINT               NULL,
    file_metadata        TEXT                 NULL,
    selection_strategy   VARCHAR(32)          NULL,
    staging_start        TIMESTAMP            NULL,
    staging_finished     TIMESTAMP            NULL,
    bringonline_token    VARCHAR(255)         NULL,
    retry_timestamp      TIMESTAMP            NULL,
    log_file             VARCHAR(2048)        NULL,
    t_log_file_debug     INTEGER              NULL,
    hashed_id            BIGINT               NULL DEFAULT 0,
    vo_name              VARCHAR(50)          NULL,
    activity             VARCHAR(255)         NULL DEFAULT 'default',
    transferred          BIGINT           NOT NULL DEFAULT 0,
    priority             INTEGER          NOT NULL DEFAULT 3,
    dest_surl_uuid       VARCHAR(36)          NULL,
    archive_start_time   TIMESTAMP            NULL,
    archive_finish_time  TIMESTAMP            NULL,
    staging_metadata     TEXT                 NULL,
    archive_metadata     TEXT                 NULL,
    scitag               INTEGER              NULL,
    src_token_id         VARCHAR(16)          NULL,
    dst_token_id         VARCHAR(16)          NULL,
    queue_id             BIGINT           NOT NULL,
    PRIMARY KEY (file_id),
    UNIQUE (dest_surl_uuid),
    CONSTRAINT job_id
        FOREIGN KEY (job_id)
        REFERENCES t_job (job_id),
    CONSTRAINT src_token_id
        FOREIGN KEY (src_token_id)
        REFERENCES t_token (token_id),
    CONSTRAINT dst_token_id
        FOREIGN KEY (dst_token_id)
        REFERENCES t_token (token_id),
    CONSTRAINT queue_id
        FOREIGN KEY (queue_id)
        REFERENCES t_queue (queue_id)
);
CREATE INDEX idx_file_job_id        ON t_file(job_id);
CREATE INDEX idx_file_activity      ON t_file(vo_name, activity);
CREATE INDEX idx_file_link_state_vo ON
    t_file(source_se,dest_se, file_state,vo_name);
CREATE INDEX idx_file_finish_time   ON t_file(finish_time);
CREATE INDEX idx_file_staging       ON t_file(file_state, vo_name, source_se);
CREATE INDEX idx_file_state_host    ON t_file(file_state, transfer_host);
CREATE INDEX idx_file_state         ON t_file(file_state);
CREATE INDEX idx_file_transfer_host ON t_file(transfer_host);
CREATE INDEX idx_file_src_token_id  ON t_file(src_token_id);
CREATE INDEX idx_file_dst_token_id  ON t_file(dst_token_id);
CREATE INDEX idx_file_queue         ON t_file(queue_id, priority, file_id);

CREATE TABLE t_file_backup (
  log_file_debug         SMALLINT             NULL,
  file_id                BIGINT           NOT NULL DEFAULT 0,
  file_index             INTEGER              NULL,
  job_id                 VARCHAR(36)      NOT NULL,
  file_state             enum_file_state  NOT NULL,
  transfer_host          VARCHAR(255)         NULL,
  source_surl            VARCHAR(1100)        NULL,
  dest_surl              VARCHAR(1100)        NULL,
  source_se              VARCHAR(255)         NULL,
  dest_se                VARCHAR(255)         NULL,
  staging_host           VARCHAR(1024)        NULL,
  reason                 VARCHAR(2048)        NULL,
  current_failures       INTEGER              NULL,
  filesize               BIGINT               NULL,
  checksum               VARCHAR(100)         NULL,
  finish_time            TIMESTAMP            NULL,
  start_time             TIMESTAMP            NULL,
  internal_file_params   VARCHAR(255)         NULL,
  pid                    INTEGER              NULL,
  tx_duration            DOUBLE PRECISION     NULL,
  throughput             REAL                 NULL,
  retry                  INTEGER          NOT NULL DEFAULT 0,
  user_filesize          BIGINT               NULL,
  file_metadata          TEXT                 NULL,
  selection_strategy     VARCHAR(32)          NULL,
  staging_start          TIMESTAMP            NULL,
  staging_finished       TIMESTAMP            NULL,
  bringonline_token      VARCHAR(255)         NULL,
  retry_timestamp        TIMESTAMP            NULL,
  log_file               VARCHAR(2048)        NULL,
  t_log_file_debug       INTEGER              NULL,
  hashed_id              INTEGER          NOT NULL DEFAULT 0,
  vo_name                VARCHAR(50)          NULL,
  activity               VARCHAR(255)     NOT NULL DEFAULT 'default',
  transferred            BIGINT           NOT NULL DEFAULT 0,
  priority               INTEGER          NOT NULL DEFAULT 3,
  dest_surl_uuid         VARCHAR(36)          NULL,
  archive_start_time     TIMESTAMP            NULL,
  archive_finish_time    TIMESTAMP            NULL,
  staging_metadata       TEXT                 NULL,
  archive_metadata       TEXT                 NULL,
  scitag                 INTEGER              NULL,
  src_token_id           VARCHAR(16)          NULL,
  dst_token_id           VARCHAR(16)          NULL,
  file_state_initial     VARCHAR(32)          NULL,
  queue_id               BIGINT               NULL
);

CREATE TABLE t_optimizer (
  source_se VARCHAR(150)     NOT NULL,
  dest_se   VARCHAR(150)     NOT NULL,
  datetime  TIMESTAMP        NOT NULL DEFAULT CURRENT_TIMESTAMP,
  ema       DOUBLE PRECISION NOT NULL DEFAULT 0,
  active    INTEGER          NOT NULL DEFAULT 2,
  nostreams INTEGER          NOT NULL DEFAULT 1,
  PRIMARY KEY (source_se,dest_se)
);
CREATE TRIGGER tr_optimizer_datetime
    BEFORE UPDATE ON t_optimizer
    FOR EACH ROW EXECUTE FUNCTION update_datetime();

CREATE TABLE t_optimizer_evolution(
    datetime        TIMESTAMP        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    source_se       VARCHAR(150)         NULL,
    dest_se         VARCHAR(150)         NULL,
    active          INTEGER              NULL,
    throughput      REAL                 NULL,
    success         REAL                 NULL,
    rationale       TEXT                 NULL,
    diff            INTEGER          NOT NULL DEFAULT 0,
    actual_active   INTEGER              NULL,
    queue_size      INTEGER              NULL,
    ema             DOUBLE PRECISION     NULL,
    filesize_avg    DOUBLE PRECISION     NULL,
    filesize_stddev DOUBLE PRECISION     NULL
);
CREATE TRIGGER tr_optimizer_evolution_datetime
    BEFORE UPDATE ON t_optimizer_evolution
    FOR EACH ROW EXECUTE FUNCTION update_datetime();
CREATE INDEX idx_optimizer_evolution_src_dst_date ON
    t_optimizer_evolution(source_se, dest_se, datetime);
CREATE INDEX dix_optimizer_evolution_datetime ON
    t_optimizer_evolution(datetime);

CREATE TABLE t_dm (
  file_id              BIGSERIAL        NOT NULL,
  job_id               VARCHAR(36)      NOT NULL,
  file_state           VARCHAR(32)      NOT NULL,
  dmHost               VARCHAR(150)         NULL,
  source_surl          VARCHAR(900)         NULL,
  dest_surl            VARCHAR(900)         NULL,
  source_se            VARCHAR(150)         NULL,
  dest_se              VARCHAR(150)         NULL,
  error_scope          VARCHAR(32)          NULL,
  error_phase          VARCHAR(32)          NULL,
  reason               VARCHAR(2048)        NULL,
  checksum             VARCHAR(100)         NULL,
  finish_time          TIMESTAMP            NULL,
  start_time           TIMESTAMP            NULL,
  internal_file_params VARCHAR(255)         NULL,
  job_finished         TIMESTAMP            NULL,
  pid                  INTEGER              NULL,
  tx_duration          DOUBLE PRECISION     NULL,
  retry                INTEGER          NOT NULL DEFAULT 0,
  user_filesize        DOUBLE PRECISION     NULL,
  file_metadata        VARCHAR(255)         NULL,
  activity             VARCHAR(255)     NOT NULL DEFAULT 'default',
  selection_strategy   VARCHAR(255)         NULL,
  dm_start             TIMESTAMP            NULL,
  dm_finished          TIMESTAMP            NULL,
  dm_token             VARCHAR(255)         NULL,
  retry_timestamp      TIMESTAMP            NULL,
  wait_timestamp       TIMESTAMP            NULL,
  wait_timeout         INTEGER              NULL,
  hashed_id            INTEGER          NOT NULL DEFAULT 0,
  vo_name              VARCHAR(100)         NULL,
  PRIMARY KEY (file_id),
  CONSTRAINT fk_dm_job_id
      FOREIGN KEY (job_id)
      REFERENCES t_job (job_id)
      ON DELETE RESTRICT ON UPDATE RESTRICT
);
CREATE INDEX idx_dm_job_id ON t_dm(job_id);

CREATE TABLE t_dm_backup (
  file_id BIGINT                        NOT NULL DEFAULT 0,
  job_id               VARCHAR(36)      NOT NULL,
  file_state           VARCHAR(32)      NOT NULL,
  dmHost               VARCHAR(150)         NULL,
  source_surl          VARCHAR(900)         NULL,
  dest_surl            VARCHAR(900)         NULL,
  source_se            VARCHAR(150)         NULL,
  dest_se              VARCHAR(150)         NULL,
  error_scope          VARCHAR(32)          NULL,
  error_phase          VARCHAR(32)          NULL,
  reason               VARCHAR(2048)        NULL,
  checksum             VARCHAR(100)         NULL,
  finish_time          TIMESTAMP            NULL,
  start_time           TIMESTAMP            NULL,
  internal_file_params VARCHAR(255)         NULL,
  job_finished         TIMESTAMP            NULL,
  pid                  INTEGER              NULL,
  tx_duration          DOUBLE PRECISION     NULL,
  retry                INTEGER          NOT NULL DEFAULT 0,
  user_filesize DOUBLE PRECISION            NULL,
  file_metadata        VARCHAR(255)         NULL,
  activity             VARCHAR(255)     NOT NULL DEFAULT 'default',
  selection_strategy   VARCHAR(255)         NULL,
  dm_start             TIMESTAMP            NULL,
  dm_finished          TIMESTAMP            NULL,
  dm_token             VARCHAR(255)         NULL,
  retry_timestamp      TIMESTAMP            NULL,
  wait_timestamp       TIMESTAMP            NULL,
  wait_timeout         INTEGER              NULL,
  hashed_id            INTEGER           NOT NULL DEFAULT 0,
  vo_name              VARCHAR(100)         NULL
);

CREATE TABLE t_file_retry_errors (
  file_id       BIGINT        NOT NULL,
  attempt       INTEGER       NOT NULL,
  datetime      TIMESTAMP         NULL,
  reason        VARCHAR(2048)     NULL,
  transfer_host VARCHAR(255)      NULL,
  log_file      VARCHAR(2048)     NULL,
  PRIMARY KEY (file_id, attempt),
  CONSTRAINT fk_file_retry_errors_file_id
      FOREIGN KEY (file_id)
      REFERENCES t_file (file_id)
      ON DELETE CASCADE ON UPDATE RESTRICT
);
CREATE INDEX idx_file_retry_errors_datetime on t_file_retry_errors(datetime);

CREATE TABLE t_authz_dn (
  dn        VARCHAR(255) NOT NULL,
  operation VARCHAR(64)  NOT NULL,
  PRIMARY KEY (dn,operation)
);

CREATE TABLE t_bad_dns (
  dn            VARCHAR(255)  NOT NULL DEFAULT '',
  message       VARCHAR(2048)     NULL,
  addition_time TIMESTAMP         NULL,
  admin_dn      VARCHAR(255)      NULL,
  status        VARCHAR(10)       NULL,
  wait_timeout  INTEGER       NOT NULL DEFAULT 0,
  PRIMARY KEY (dn)
);

CREATE TABLE t_hosts (
  hostname               VARCHAR(64) NOT NULL,
  beat                   TIMESTAMP       NULL,
  drain                  INTEGER         NULL DEFAULT 0,
  service_name           VARCHAR(64) NOT NULL,
  max_url_copy_processes INTEGER         NULL,
  PRIMARY KEY (hostname, service_name)
);

CREATE TABLE t_schema_vers (
  major   INTEGER NOT NULL,
  minor   INTEGER NOT NULL,
  patch   INTEGER NOT NULL,
  message TEXT  NULL,
  PRIMARY KEY (major, minor, patch)
);
INSERT INTO t_schema_vers (major, minor, patch, message)
VALUES (9, 1, 0, 'Schema 9.1.0');

CREATE TABLE t_bad_ses (
  se            VARCHAR(256) NOT NULL DEFAULT '',
  message       VARCHAR(2048)    NULL,
  addition_time TIMESTAMP        NULL,
  admin_dn      VARCHAR(255)     NULL,
  vo            VARCHAR(100)     NULL,
  status        VARCHAR(10)      NULL,
  wait_timeout  INTEGER      NOT NULL DEFAULT 0,
  PRIMARY KEY (se)
);

CREATE TABLE t_server_config (
  retry          INTEGER      NOT NULL DEFAULT 0,
  max_time_queue INTEGER      NOT NULL DEFAULT 0,
  sec_per_mb     INTEGER      NOT NULL DEFAULT 0,
  global_timeout INTEGER      NOT NULL DEFAULT 0,
  vo_name        VARCHAR(100)     NULL,
  no_streaming   VARCHAR(3)       NULL,
  show_user_dn   VARCHAR(3)       NULL
);
INSERT INTO t_server_config (vo_name)
VALUES ('*');

CREATE TABLE t_se (
  storage                 VARCHAR(150) NOT NULL,
  site                    VARCHAR(45)      NULL,
  metadata                TEXT             NULL,
  ipv6                    SMALLINT         NULL,
  udt                     SMALLINT         NULL,
  debug_level             INTEGER          NULL,
  inbound_max_active      INTEGER          NULL,
  inbound_max_throughput  REAL             NULL,
  outbound_max_active     INTEGER          NULL,
  outbound_max_throughput REAL             NULL,
  eviction                VARCHAR(1)       NULL,
  tpc_support             VARCHAR(10)      NULL,
  skip_eviction           VARCHAR(1)       NULL,
  tape_endpoint           VARCHAR(1)       NULL,
  overwrite_disk_enabled  VARCHAR(1)       NULL,
  PRIMARY KEY (storage)
);
INSERT INTO t_se (storage, inbound_max_active, outbound_max_active)
VALUES ('*', 200, 200);

CREATE TABLE t_stage_req (
  vo_name        VARCHAR(100) NOT NULL,
  host           VARCHAR(150) NOT NULL,
  operation      VARCHAR(150) NOT NULL,
  concurrent_ops INTEGER      NOT NULL DEFAULT 0,
  PRIMARY KEY (vo_name, host, operation)
);

CREATE TABLE t_link_config (
  source_se        VARCHAR(150) NOT NULL,
  dest_se          VARCHAR(150) NOT NULL,
  symbolic_name    VARCHAR(150) NOT NULL,
  min_active       INTEGER          NULL,
  max_active       INTEGER          NULL,
  optimizer_mode   INTEGER          NULL,
  tcp_buffer_size  INTEGER          NULL,
  nostreams        INTEGER          NULL,
  no_delegation    VARCHAR(3)       NULL,
  third_party_turl VARCHAR(150)     NULL,
  PRIMARY KEY (source_se,dest_se),
  UNIQUE (symbolic_name)
);
INSERT INTO t_link_config (
    source_se,
    dest_se,
    symbolic_name,
    min_active,
    max_active,
    optimizer_mode,
    nostreams,
    no_delegation
) VALUES (
    '*',
    '*',
    '*',
    2,
    130,
    2,
    0,
    'off'
);

CREATE TABLE t_share_config (
  source      VARCHAR(150) NOT NULL,
  destination VARCHAR(150) NOT NULL,
  vo          VARCHAR(100) NOT NULL,
  active      INTEGER      NOT NULL,
  PRIMARY KEY (source,destination,vo),
  CONSTRAINT fk_share_config_source_destination
      FOREIGN KEY (source, destination)
      REFERENCES t_link_config (source_se, dest_se)
      ON DELETE RESTRICT ON UPDATE RESTRICT
);

CREATE TABLE t_activity_share_config (
  vo             VARCHAR(100)  NOT NULL,
  activity_share VARCHAR(1024) NOT NULL,
  active         VARCHAR(3)        NULL,
  PRIMARY KEY (vo)
);

CREATE TABLE t_config_audit (
  datetime TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  dn       VARCHAR(255)      NULL,
  config   VARCHAR(4000)     NULL,
  action   VARCHAR(100)      NULL
);
CREATE TRIGGER tr_config_audit_datetime
    BEFORE UPDATE ON t_config_audit
    FOR EACH ROW EXECUTE FUNCTION update_datetime();

CREATE TABLE t_cloudStorage (
  cloudStorage_name VARCHAR(150)  NOT NULL,
  app_key           VARCHAR(255)      NULL,
  app_secret        VARCHAR(255)      NULL,
  service_api_url   VARCHAR(1024)     NULL,
  PRIMARY KEY (cloudStorage_name)
);

CREATE TABLE t_cloudStorageUser (
  user_dn              VARCHAR(700) NOT NULL DEFAULT '',
  vo_name              VARCHAR(100) NOT NULL DEFAULT '',
  cloudStorage_name    VARCHAR(150) NOT NULL,
  access_token         VARCHAR(255)     NULL,
  access_token_secret  VARCHAR(255)     NULL,
  request_token        VARCHAR(255)     NULL,
  request_token_secret VARCHAR(255)     NULL,
  PRIMARY KEY (user_dn, vo_name, cloudStorage_name),
  CONSTRAINT fk_cloudStorageUser_cloudStorage_name
      FOREIGN KEY (cloudStorage_name)
      REFERENCES t_cloudStorage (cloudStorage_name)
      ON DELETE RESTRICT ON UPDATE RESTRICT
);
CREATE INDEX idx_cloudStorageUser_cloudStorage_name ON
    t_cloudStorageUser(cloudStorage_name);

CREATE TABLE t_oauth2_apps (
  client_id     VARCHAR(64)   NOT NULL,
  client_secret VARCHAR(128)  NOT NULL,
  owner         VARCHAR(1024) NOT NULL,
  name          VARCHAR(128)  NOT NULL,
  description   VARCHAR(512)      NULL,
  website       VARCHAR(1024)     NULL,
  redirect_to   VARCHAR(4096)     NULL,
  PRIMARY KEY (client_id)
);

CREATE TABLE t_oauth2_codes (
  client_id VARCHAR(64)      NULL,
  code      VARCHAR(128) NOT NULL,
  scope     VARCHAR(512)     NULL,
  dlg_id    VARCHAR(100) NOT NULL,
  PRIMARY KEY (code)
);

CREATE TABLE t_oauth2_providers (
  provider_url VARCHAR(250)  NOT NULL,
  provider_jwk VARCHAR(1000) NOT NULL,
  PRIMARY KEY (provider_url)
);

CREATE TABLE t_oauth2_tokens (
  client_id     VARCHAR(64)  NOT NULL,
  scope         VARCHAR(512)     NULL,
  access_token  VARCHAR(128)     NULL,
  token_type    VARCHAR(64)      NULL,
  expires       TIMESTAMP        NULL,
  refresh_token VARCHAR(128)     NULL,
  dlg_id        VARCHAR(100)     NULL,
  PRIMARY KEY (client_id)
);

CREATE OR REPLACE FUNCTION inc_queue_counter (
    _vo_name varchar,
    _source_se varchar,
    _dest_se varchar,
    _activity varchar,
    _file_state enum_file_state,
    _delta bigint
) RETURNS bigint
AS $$
DECLARE
    _queue_id bigint := NULL;
BEGIN
    -- Assume the t_queue row already exists
    UPDATE
        t_queue
    SET
        nb_files = nb_files + _delta
    WHERE
        vo_name = _vo_name
    AND
        source_se = _source_se
    AND
        dest_se = _dest_se
    AND
        activity = _activity
    AND
        file_state = _file_state
    RETURNING
        queue_id INTO _queue_id;

    IF FOUND THEN
        RETURN _queue_id;
    END IF;

    -- The t_queue row does not exist so create one
    INSERT INTO t_queue (
        vo_name,
        source_se,
        dest_se,
        activity,
        file_state,
        nb_files
    ) VALUES (
        _vo_name,
        _source_se,
        _dest_se,
        _activity,
        _file_state,
        _delta
    )
    ON CONFLICT (vo_name, source_se, dest_se, activity, file_state) DO
        UPDATE SET nb_files =
            t_queue.nb_files + EXCLUDED.nb_files
    RETURNING
        queue_id INTO _queue_id;

    RETURN _queue_id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION schedule_next_file_in_queue(
    _submitted_queue_id bigint
) RETURNS bigint
AS $$
DECLARE
    _file_row_file_id bigint;
    _file_row_file_state enum_file_state;
    _file_changed boolean := FALSE;
BEGIN
    -- Returns the file_id of the scheduled file-transfer else NULL

    SELECT file_id, file_state
    INTO _file_row_file_id, _file_row_file_state
    FROM
        t_file
    WHERE
        queue_id = _submitted_queue_id
    ORDER BY
        queue_id, priority, file_id
    LIMIT 1
    FOR UPDATE;

    IF NOT FOUND THEN
        RAISE 'schedule_next_file_in_queue failed: Empty queue: queue_id=%',
            _submitted_queue_id;
    END IF;

    IF _file_row_file_state != 'SUBMITTED' THEN
        RAISE 'schedule_next_file_in_queue failed: Initial file state is not SUBMITTED: file_id=% file_state=%',
            _file_row_file_id, _file_row_file_state;
    END IF;

    UPDATE
        t_file
    SET
        file_state = 'SCHEDULED'
    WHERE
        file_id = _file_row_file_id;

    IF NOT FOUND THEN
        RETURN NULL;
    ELSE
        RETURN _file_row_file_id;
    END IF;
END;
$$ LANGUAGE plpgsql;

CREATE TYPE transfer_to_start AS (
    file_state          VARCHAR(32),
    source_surl         VARCHAR(1100),
    dest_surl           VARCHAR(1100),
    job_id              VARCHAR(36),
    vo_name             VARCHAR(50),
    file_id             BIGINT,
    overwrite_flag      VARCHAR(1),
    archive_timeout     INTEGER,
    dst_file_report     VARCHAR(1),
    user_dn             VARCHAR(1024),
    cred_id             VARCHAR(16),
    src_token_id        VARCHAR(16),
    dst_token_id        VARCHAR(16),
    checksum            VARCHAR(100),
    checksum_method     VARCHAR(1),
    source_space_token  VARCHAR(255),
    space_token         VARCHAR(255),
    copy_pin_lifetime   INTEGER,
    bring_online        INTEGER,
    file_metadata       TEXT,
    archive_metadata    TEXT,
    job_metadata        TEXT,
    user_filesize       BIGINT,
    file_index          INTEGER,
    bringonline_token   VARCHAR(255),
    scitag              INTEGER,
    activity            VARCHAR(255),
    source_se           VARCHAR(255),
    dest_se             VARCHAR(255),
    selection_strategy  VARCHAR(32),
    internal_job_params VARCHAR(255),
    job_type            VARCHAR(1)
);

CREATE OR REPLACE FUNCTION get_transfers_to_start(
    _nb_files bigint
) RETURNS SETOF transfer_to_start
AS $$
DECLARE
    _file_id bigint;
    _transfer_to_start transfer_to_start;
    _file_changed boolean := FALSE;
BEGIN
    -- Returns list of file transfers to be started
    FOR _file_id IN
        -- Select list of files to update
        SELECT
            file_id
        FROM
            t_file
        WHERE
            file_state='SCHEDULED'
        ORDER BY
            queue_id, priority, file_id
        FOR UPDATE SKIP LOCKED
        LIMIT
            _nb_files

    -- Loop over files to update
    LOOP
        -- Update file_state and queues
        UPDATE
            t_file
        SET
            file_state = 'SELECTED'
        WHERE
            file_id = _file_id;

        -- If file state was not changed it means it was already in SELECTED state. Something must have gone wrong
        IF NOT FOUND THEN
            RAISE 'get_transfers_to_start panic: File was already in SELECTED state: file_id=%',
            _file_id;
        END IF;

        -- Select data and assign it to _transfer_to_start
        SELECT
            f.file_state,
            f.source_surl,
            f.dest_surl,
            f.job_id,
            j.vo_name,
            f.file_id,
            j.overwrite_flag,
            j.archive_timeout,
            j.dst_file_report,
            j.user_dn,
            j.cred_id,
            f.src_token_id,
            f.dst_token_id,
            f.checksum,
            j.checksum_method,
            j.source_space_token,
            j.space_token,
            j.copy_pin_lifetime,
            j.bring_online,
            f.file_metadata,
            f.archive_metadata,
            j.job_metadata,
            f.user_filesize,
            f.file_index,
            f.bringonline_token,
            f.scitag,
            f.activity,
            f.source_se,
            f.dest_se,
            f.selection_strategy,
            j.internal_job_params,
            j.job_type
        INTO _transfer_to_start
        FROM
            t_file as f
        INNER JOIN
            t_job as j
        ON
            j.job_id = f.job_id
        WHERE
            f.file_id = _file_id;

        -- Return the current row of transfer_to_start
        RETURN NEXT _transfer_to_start;

    END LOOP;

    RETURN;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION update_queue_count()
    RETURNS TRIGGER AS $$
DECLARE
    _curr_queue_id bigint := OLD.queue_id;
    _next_queue_id bigint;
BEGIN
    -- Get the next queue_id based on the queue_id key (vo_name, source_se, dest_se, activity, file_state).
    SELECT
        queue_id
    INTO
        _next_queue_id
    FROM
        t_queue
    WHERE
        vo_name = NEW.vo_name
    AND
        source_se = NEW.source_se
    AND
        dest_se = NEW.dest_se
    AND
        activity = NEW.activity
    AND
        file_state = NEW.file_state;

    -- If the next queue already exists
    IF FOUND THEN
        -- Avoid deadlock by locking the current and next queues in queue_id order
        IF _curr_queue_id < _next_queue_id THEN
            SELECT queue_id INTO _curr_queue_id
            FROM
                t_queue
            WHERE
                queue_id = _curr_queue_id
            FOR UPDATE;
            SELECT queue_id INTO _next_queue_id
            FROM
                t_queue
            WHERE
                queue_id = _next_queue_id
            FOR UPDATE;
        ELSE
            SELECT queue_id INTO _next_queue_id
            FROM
                t_queue
            WHERE
                queue_id = _next_queue_id
            FOR UPDATE;
            SELECT queue_id INTO _curr_queue_id
            FROM
                t_queue
            WHERE
                queue_id = _curr_queue_id
            FOR UPDATE;
        END IF;
    END IF;

    -- Decrement the current queue counter
    UPDATE
        t_queue
    SET
        nb_files = nb_files - 1
    WHERE
        queue_id = _curr_queue_id;

    IF NOT FOUND THEN
        RAISE 'update_queue_count failed: Failed to decrement counter of current queue: queue_id=%',
            _curr_queue_id;
    END IF;

    SELECT inc_queue_counter(
                   _vo_name => NEW.vo_name,
                   _source_se => NEW.source_se,
                   _dest_se => NEW.dest_se,
                   _activity => NEW.activity,
                   _file_state => NEW.file_state,
                   _delta => 1
               ) INTO _next_queue_id;

    -- Update the t_file row with the new queue_id
    UPDATE
        t_file
    SET
        queue_id = _next_queue_id
    WHERE
        file_id = NEW.file_id;

    IF NOT FOUND THEN
        RAISE 'update_queue_count failed: Failed to update file transfer row: file_id=%',
            NEW.file_id;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;


CREATE TRIGGER trg_update_queue_count
    AFTER UPDATE OF vo_name, source_se, dest_se, activity, file_state ON t_file
    FOR EACH ROW
EXECUTE FUNCTION update_queue_count();
