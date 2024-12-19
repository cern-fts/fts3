"""
The file-transfer scheduler
"""

import socket
import time
import db


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

        sched_input = db.get_scheduler_input_from_db(dbconn, self._algo_opaque_data)
        self.log.debug(f"sched_input={sched_input}")
        algo = self._algo_class(sched_input)
        sched_output = algo.schedule()
        self.log.debug(f"sched_output={sched_output}")
        if not sched_output:
            return

        self._algo_opaque_data = sched_output.get_opaque_data()
        self._write_scheduler_output_to_db(dbconn, sched_output)
        nb_scheduled = sched_output.get_total_nb_transfers()
        if nb_scheduled:
            self.log.info(f"Scheduled transfers: nb_scheduled={nb_scheduled}")

    @staticmethod
    def _this_host_is_scheduling(dbconn) -> bool:
        try:
            worker_hostname, _ = Scheduler._select_worker_hostname(dbconn)
            return worker_hostname and worker_hostname == socket.gethostname()
        except Exception as ex:
            raise Exception(f"Scheduler._this_host_is_scheduling(): {ex}") from ex

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

    def _write_scheduler_output_to_db(self, dbconn, sched_output):
        try:
            transfers_per_queue = sched_output.get_transfers_per_queue()
            for queue_id, nb_transfers in transfers_per_queue.items():
                for _ in range(nb_transfers):
                    self._schedule_next_file_in_queue(dbconn, queue_id)
        except Exception as ex:
            raise Exception(f"Scheduler._write_scheduler_output_to_db(): {ex}") from ex

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
