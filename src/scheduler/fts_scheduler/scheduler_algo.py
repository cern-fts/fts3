class SchedulerOutput:
    def __init__(self):
        self._transfers_per_queue = {}
        self._total_nb_transfers = 0
        self._opaque_data = (
            None  # # Opaque data passed from one round of scheduling to the next
        )

    def __str__(self):
        return (
            "{"
            + f"transfers_per_queue={self._transfers_per_queue},"
            + f"total_nb_transfers={self._total_nb_transfers},"
            + f"opaque_data={self._opaque_data}"
            + "}"
        )

    def inc_transfers_for_queue(self, queue_id, nb_transfers):
        if queue_id in self._transfers_per_queue.keys():
            self._transfers_per_queue[queue_id] += nb_transfers
        else:
            self._transfers_per_queue[queue_id] = nb_transfers
        self._total_nb_transfers += nb_transfers

    def get_transfers_per_queue(self):
        return self._transfers_per_queue

    def get_total_nb_transfers(self):
        return self._total_nb_transfers

    def set_opaque_data(self, opaque_data):
        self._opaque_data = opaque_data

    def get_opaque_data(self):
        return self._opaque_data


class SchedulerAlgo:
    def __init__(self, sched_input):
        self.sched_input = sched_input

    def schedule(self) -> SchedulerOutput:
        pass
