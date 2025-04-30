"""
The file-transfer scheduler
"""

import socket
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
        db.write_scheduler_output_to_db(dbconn, sched_output)
        nb_scheduled = sched_output.get_total_nb_transfers()
        if nb_scheduled:
            self.log.info(f"Scheduled transfers: nb_scheduled={nb_scheduled}")

    @staticmethod
    def _this_host_is_scheduling(dbconn) -> bool:
        worker_hostname, _ = db.get_scheduler_fqdn_from_db(dbconn)
        return worker_hostname and worker_hostname == socket.gethostname()
