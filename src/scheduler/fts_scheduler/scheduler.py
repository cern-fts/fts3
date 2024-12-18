"""
The file-transfer scheduler
"""

import json
import socket
import time


class Scheduler:  # pylint:disable=too-few-public-methods
    """
    Thefile-transfer scheduler
    """

    def __init__(self, log, scheduler_algo_class):
        self.log = log
        self._algo_class = scheduler_algo_class
        self._algo_opaque_data = (
            None  # Opaque data passed from one round of scheduling to the next
        )

    def schedule(self, dbconn):
        """
        Runs a single round the file-trasfer scheduler
        """
        if not self._this_host_is_scheduling(dbconn):
            return

        sched_input = self._get_sched_input(dbconn)
        self.log.debug(f"sched_input={sched_input}")
        algo = self._algo_class(sched_input)
        scheduler_decision = algo.schedule()
        self.log.debug(f"scheduler_decision={scheduler_decision}")
        if not scheduler_decision:
            return

        self._algo_opaque_data = scheduler_decision.get_opaque_data()
        self._write_scheduler_decision_to_db(dbconn, scheduler_decision)
        nb_scheduled = scheduler_decision.get_total_nb_transfers()
        if nb_scheduled:
            self.log.info(f"Scheduled transfers: nb_scheduled={nb_scheduled}")

    @staticmethod
    def _get_queues(dbconn):
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
            raise Exception(f"Scheduler._get_queues(): {ex}") from ex

    @staticmethod
    def _get_max_url_copy_processes(dbconn) -> int:
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
            raise Exception(f"Scheduler._get_max_url_copy_processes(): {ex}") from ex

    @staticmethod
    def _get_link_limits(dbconn):
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

    @staticmethod
    def _get_active_stats(dbconn):
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
            raise Exception(f"Scheduler._get_active_stats: {ex}") from ex

    @staticmethod
    def _get_storage_limits(dbconn):
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

            storages = {}
            for row in rows:
                storage = {}
                storage["storage"] = row[0]
                storage["inbound_max_active"] = row[1]
                storage["outbound_max_active"] = row[2]

                storage_key = storage["storage"]
                storages[storage_key] = storage

            return storages, db_sec
        except Exception as ex:
            raise Exception(f"Scheduler._get_storage_limits(): {ex}") from ex

    @staticmethod
    def _get_optimizer_limits(dbconn):
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
            raise Exception(f"Scheduler._optimizer_max_active: {ex}") from ex

    @staticmethod
    def _get_link_vo_shares(dbconn):
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
            raise Exception(f"Scheduler._get_link_vo_shares(): {ex}") from ex

    @staticmethod
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
            raise Exception(f"Scheduler._activity_share_list_to_dict(): {ex}") from ex

    @staticmethod
    def _get_vo_activity_shares(dbconn):
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
                vo_activity_shares[vo_name] = Scheduler._activity_share_list_to_dict(
                    activity_shares_list
                )

            return vo_activity_shares, db_sec
        except Exception as ex:
            raise Exception(f"Scheduler._get_vo_activity_shares(): {ex}") from ex

    def _get_sched_input(self, dbconn):
        try:
            sched_input = {}
            sched_input["opaque_data"] = self._algo_opaque_data
            sched_input["max_url_copy_processes"], _ = (
                Scheduler._get_max_url_copy_processes(dbconn)
            )
            sched_input["queues"], _ = Scheduler._get_queues(dbconn)
            sched_input["link_limits"], _ = Scheduler._get_link_limits(dbconn)
            sched_input["active_stats"], _ = Scheduler._get_active_stats(dbconn)
            sched_input["optimizer_limits"], _ = Scheduler._get_optimizer_limits(dbconn)
            sched_input["storages_limits"], _ = Scheduler._get_storage_limits(dbconn)
            sched_input["vo_activity_shares"], _ = Scheduler._get_vo_activity_shares(
                dbconn
            )
            sched_input["link_vo_shares"], _ = Scheduler._get_link_vo_shares(dbconn)

            return sched_input
        except Exception as ex:
            raise Exception(f"Scheduler._get_sched_input(): {ex}") from ex

    def _schedule_next_file_in_queue(self, dbconn, submitted_queue_id):
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
                    self.log.debug(
                        "Scheduled file: "
                        f"file_id={file_id} queue_id={submitted_queue_id} db_sec={db_sec}"
                    )
        except Exception as ex:
            raise Exception(f"Scheduler._schedule_next_file_in_queue(): {ex}") from ex

    @staticmethod
    def _select_worker_hostname(dbconn):
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
            raise Exception(f"Scheduler._select_worker_hostname(): {ex}") from ex

    @staticmethod
    def _this_host_is_scheduling(dbconn) -> bool:
        try:
            worker_hostname, _ = Scheduler._select_worker_hostname(dbconn)
            return worker_hostname and worker_hostname == socket.gethostname()
        except Exception as ex:
            raise Exception(f"Scheduler._this_host_is_scheduling(): {ex}") from ex

    def _write_scheduler_decision_to_db(self, dbconn, scheduler_decision):
        try:
            transfers_per_queue = scheduler_decision.get_transfers_per_queue()
            for queue_id, nb_transfers in transfers_per_queue.items():
                for _ in range(nb_transfers):
                    self._schedule_next_file_in_queue(dbconn, queue_id)
        except Exception as ex:
            raise Exception(
                f"Scheduler._write_scheduler_decision_to_db(): {ex}"
            ) from ex
