"""
Database access-layer of the file-transfer scheduler
"""

import json
import time
import psycopg2
import psycopg2.pool

from scheduler_algo import SchedulerInput, StorageLimits, StorageInOutLimits


class DbConn:
    """
    Wrapper around a psycopg2 connection object that was obtained from a
    psycopg2.pool.ThreadedConnectionPool.  This wrapper automatically returns
    the connection to the pool when it is closed.
    """

    def __init__(self, pool, dbconn):
        self.open = True
        self.pool = pool
        self.dbconn = dbconn

    def close(self):
        """Returns this database connection to its pool"""
        if self.open:
            self.pool.putconn(self.dbconn)
            self.open = False

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def cursor(self):
        """
        Returns the cursor of this database connection
        """
        if self.open:
            return self.dbconn.cursor()
        raise Exception("Failed to get cursor from connection: Connection closed")


class DbConnPool:  # pylint:disable=too-few-public-methods
    """
    Wrapper around psycopg2.pool.ThreadedConnectionPool which adds get_dbconn()
    to return DbConn objects which in turn wrap connection objects so that they
    are automatically returned to the pool when they are closed.  The
    get_dbconn() also switched autocommit on by default.
    """

    def __init__(
        self, min_conn, max_conn, host, port, db_name, user, password, sslmode
    ):  # pylint:disable=too-many-arguments
        self.pool = psycopg2.pool.ThreadedConnectionPool(
            minconn=min_conn,
            maxconn=max_conn,
            host=host,
            port=port,
            dbname=db_name,
            user=user,
            password=password,
            sslmode=sslmode,
        )

    def get_dbconn(self):
        """
        Returns a database connection from the pool with autocommit set to True
        """
        dbconn = self.pool.getconn()
        dbconn.set_session(autocommit=True)
        return DbConn(self.pool, dbconn)


def get_scheduler_input_from_db(dbconn, opaque_data) -> SchedulerInput:
    """
    Queries the FTS database for scheduler input and returns the result as a SchedulerInput object
    """
    return SchedulerInput(
        opaque_data=opaque_data,
        max_url_copy_processes=_get_max_url_copy_processes_from_db(dbconn)[0],
        queues=_get_queues_from_db(dbconn)[0],
        link_limits=_get_link_limits_from_db(dbconn)[0],
        active_stats=_get_active_stats_from_db(dbconn)[0],
        optimizer_limits=_get_optimizer_limits_from_db(dbconn)[0],
        storage_limits=_get_storage_limits_from_db(dbconn)[0],
        vo_activity_shares=_get_vo_activity_shares_from_db(dbconn)[0],
        link_vo_shares=_get_link_vo_shares_from_db(dbconn)[0],
    )


def _get_max_url_copy_processes_from_db(dbconn) -> int:
    """
    Queries the FTS database for the maximum number of concurrent fts_url_copy processes
    """
    try:
        sql = """
            SELECT
                COALESCE(SUM(max_url_copy_processes), 0)
            FROM
                t_hosts
            WHERE
                service_name = 'fts_server'
            AND
                drain != 1
        """
        start_db = time.time()
        cursor = dbconn.cursor()
        cursor.execute(sql)
        rows = cursor.fetchall()
        db_sec = time.time() - start_db

        if not rows:
            raise Exception("SELECT returned no rows")

        max_url_copy_processes = rows[0][0]
        return max_url_copy_processes, db_sec
    except Exception as ex:
        raise Exception(
            f"SchedulerInputFromDb._get_max_url_copy_processes(): {ex}"
        ) from ex


def _get_queues_from_db(dbconn):
    """
    Queries the FTS database for the queues of pending file-transfer requests
    """
    try:
        sql = """
            SELECT
                queue_id,
                vo_name,
                source_se,
                dest_se,
                activity,
                nb_files
            FROM
                t_queue
            WHERE
                file_state = 'SUBMITTED'
            AND
            nb_files > 0
        """
        start_db = time.time()
        cursor = dbconn.cursor()
        cursor.execute(sql)
        rows = cursor.fetchall()
        db_sec = time.time() - start_db

        queues = {}
        for row in rows:
            queue = {
                "queue_id": row[0],
                "vo_name": row[1],
                "source_se": row[2],
                "dest_se": row[3],
                "activity": row[4],
                "nb_files": row[5],
            }
            queue_id = queue["queue_id"]
            queues[queue_id] = queue
        return queues, db_sec
    except Exception as ex:
        raise Exception(f"SchedulerInputFromDb._get_queues(): {ex}") from ex


def _get_link_limits_from_db(dbconn):
    try:
        sql = """
            SELECT
                source_se,
                dest_se,
                min_active,
                max_active
            FROM
                t_link_config
        """
        start_db = time.time()
        cursor = dbconn.cursor()
        cursor.execute(sql)
        rows = cursor.fetchall()
        db_sec = time.time() - start_db

        links = {}
        for row in rows:
            link = {}
            link["source_se"] = row[0]
            link["dest_se"] = row[1]
            link["min_active"] = row[2]
            link["max_active"] = row[3]

            link_key = (link["source_se"], link["dest_se"])
            links[link_key] = link

        return links, db_sec
    except Exception as ex:
        raise Exception(f"Schedduler._get_link_limits(): {ex}") from ex


def _get_active_stats_from_db(dbconn):
    """
    Returns the number of active tranfers per acivity per VO per link.
    A transfer is considered to be ACTIVE if is actually ACTIVE or if it has been scheduled and
    is on its way to becoming ACTIVE, in other words if is SCHEDULED, SELECTED or READY.
    """
    try:
        sql = """
            SELECT
                source_se,
                dest_se,
                vo_name,
                activity,
                SUM(nb_files)::bigint as nb_active
            FROM
                t_queue
            WHERE
                file_state IN ('SCHEDULED', 'SELECTED', 'READY', 'ACTIVE')
            GROUP BY
                source_se,
                dest_se,
                vo_name,
                activity
        """
        start_db = time.time()
        cursor = dbconn.cursor()
        cursor.execute(sql)
        rows = cursor.fetchall()
        db_sec = time.time() - start_db

        result = []
        for row in rows:
            stats = {}
            stats["source_se"] = row[0]
            stats["dest_se"] = row[1]
            stats["vo_name"] = row[2]
            stats["activity"] = row[3]
            stats["nb_active"] = row[4]

            result.append(stats)

        return result, db_sec
    except Exception as ex:
        raise Exception(f"SchedulerInputFromDb._get_active_stats: {ex}") from ex


def _get_storage_limits_from_db(dbconn):
    try:
        sql = """
            SELECT
                storage,
                inbound_max_active,
                outbound_max_active
            FROM
                t_se
        """
        start_db = time.time()
        cursor = dbconn.cursor()
        cursor.execute(sql)
        rows = cursor.fetchall()
        db_sec = time.time() - start_db

        storage_to_limits = {}
        for row in rows:
            storage = row[0]
            limits = StorageInOutLimits(
                inbound_max_active=row[1], outbound_max_active=row[2]
            )
            storage_to_limits[storage] = limits

        return StorageLimits(storage_to_limits), db_sec
    except Exception as ex:
        raise Exception(f"_get_storage_limits_from_db(): {ex}") from ex


def _get_optimizer_limits_from_db(dbconn):
    """
    Returns a map from link to the maximum number of active transfers the link can have as
    decided by the FTS optimizer
    """
    try:
        sql = """
            SELECT
                source_se,
                dest_se,
                active
            FROM
                t_optimizer
        """
        start_db = time.time()
        cursor = dbconn.cursor()
        cursor.execute(sql)
        rows = cursor.fetchall()
        db_sec = time.time() - start_db

        links = {}
        for row in rows:
            link = {}
            link["source_se"] = row[0]
            link["dest_se"] = row[1]
            link["active"] = row[2]

            link_key = (link["source_se"], link["dest_se"])
            links[link_key] = link

        return links, db_sec
    except Exception as ex:
        raise Exception(f"SchedulerInputFromDb._get_optimizer_max_active: {ex}") from ex


def _get_vo_activity_shares_from_db(dbconn):
    try:
        sql = """
            SELECT
                vo,
                activity_share
            FROM
                t_activity_share_config
            WHERE
                active = 'on'
        """
        start_db = time.time()
        cursor = dbconn.cursor()
        cursor.execute(sql)
        rows = cursor.fetchall()
        db_sec = time.time() - start_db

        vo_activity_shares = {}
        for row in rows:
            vo_name = row[0]
            activity_shares_list = json.loads(row[1])
            vo_activity_shares[vo_name] = _activity_share_list_to_dict(
                activity_shares_list
            )

        return vo_activity_shares, db_sec
    except Exception as ex:
        raise Exception(f"SchedulerInputFromDb._get_vo_activity_shares(): {ex}") from ex


def _activity_share_list_to_dict(activity_share_list):
    try:
        activity_share_dict = {}
        for activity_weight_pair in activity_share_list:
            if len(activity_weight_pair.keys()) != 1:
                raise Exception(
                    "Invalid number of keys in activity to weight pair: "
                    f"expected=1 found={len(activity_weight_pair.keys())}"
                )
            activity = next(iter(activity_weight_pair))
            weight = activity_weight_pair[activity]
            if activity in activity_share_dict:
                raise Exception("Duplicate activity: activity={activity}")
            activity_share_dict[activity] = weight
        return activity_share_dict
    except Exception as ex:
        raise Exception(
            f"SchedulerInputFromDb._activity_share_list_to_dict(): {ex}"
        ) from ex


def _get_link_vo_shares_from_db(dbconn):
    try:
        sql = """
            SELECT
                source,
                destination,
                vo,
                active
            FROM
                t_share_config
        """
        start_db = time.time()
        cursor = dbconn.cursor()
        cursor.execute(sql)
        rows = cursor.fetchall()
        db_sec = time.time() - start_db

        link_vo_shares = {}
        for row in rows:
            vo_share = {}
            vo_share["source"] = row[0]
            vo_share["destination"] = row[1]
            vo_share["vo"] = row[2]
            vo_share["active"] = row[3]

            link_key = (vo_share["source"], vo_share["destination"])
            if link_key not in link_vo_shares:
                link_vo_shares[link_key] = []
            link_vo_shares[link_key].append(vo_share)

        return link_vo_shares, db_sec
    except Exception as ex:
        raise Exception(f"SchedulerInputFromDb._get_link_vo_shares(): {ex}") from ex


def get_scheduler_fqdn_from_db(dbconn):
    """
    Returns the fully qualified domain name of the host currently chosen as the one run the
    file-transfer scheduler
    """
    try:
        sql = """
            SELECT
              hostname
            FROM
              t_hosts
            WHERE
              drain IS NOT NULL OR drain = 0
            ORDER BY
              hostname ASC
            LIMIT 1
        """
        start_db = time.time()
        cursor = dbconn.cursor()
        cursor.execute(sql)
        row = cursor.fetchone()
        db_sec = time.time() - start_db

        if row:
            hostname = row[0]
        else:
            hostname = None

        return hostname, db_sec
    except Exception as ex:
        raise Exception(f"get_scheduler_hostname_from_db(): {ex}") from ex


def write_scheduler_output_to_db(dbconn, sched_output):
    """
    Writes the specified output from the file-transfer scheduler to the database
    """
    try:
        transfers_per_queue = sched_output.get_transfers_per_queue()
        for queue_id, nb_transfers in transfers_per_queue.items():
            for _ in range(nb_transfers):
                _call_schedule_next_file_in_queue(dbconn, queue_id)
    except Exception as ex:
        raise Exception(f"write_scheduler_output_to_db(): {ex}") from ex


def _call_schedule_next_file_in_queue(dbconn, submitted_queue_id):
    """
    Calls the schedule_next_file_in_queue() database function
    """
    try:
        sql = """
            SELECT schedule_next_file_in_queue(
                _submitted_queue_id => %(submitted_queue_id)s
            ) AS file_id
        """
        params = {"submitted_queue_id": submitted_queue_id}
        start_db = time.time()
        with dbconn.cursor() as cursor:
            cursor.execute(sql, params)
            rows = cursor.fetchall()
            db_sec = time.time() - start_db

            if len(rows) > 0:
                file_id = rows[0][0]
                return file_id, db_sec
            return None, db_sec
        return None, None
    except Exception as ex:
        raise Exception(f"_call_schedule_next_file_in_queue(): {ex}") from ex
