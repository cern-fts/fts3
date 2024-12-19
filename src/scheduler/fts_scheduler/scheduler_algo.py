"""
Scheduler-algorithm inputs and outputs
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any


@dataclass
class StorageInOutLimits:
    """
    The configured limits of a storage-endpoint
    """

    inbound_max_active: int
    outbound_max_active: int


@dataclass
class StorageLimits:
    """
    Configured storage end-point limits.  The main purpose of this class is to provide the following
    two convenience methods which encapsulate the concept of the default storage-endpoint named "*"
    * get_inbound_max_active()
    * get_outbound_max_active()
    """

    storage_to_limits: dict[str, StorageInOutLimits]

    def get_inbound_max_active(self, storage):
        """
        Returns the configured maximum number of inbound concurrent file-transfers for the given
        storage-endpoint
        """
        if storage in self.storage_to_limits:
            return self.storage_to_limits[storage].inbound_max_active
        if "*" in self.storage_to_limits:
            return self.storage_to_limits["*"].inbound_max_active
        raise Exception(
            "StorageLimits.get_storage_inbound_max_active(): "
            f"No storage-limit configuration: storage={storage} or storage=*"
        )

    def get_outbound_max_active(self, storage):
        """
        Returns the configured maximum number of outbound concurrent file-transfers for the given
        storage-endpoint
        """
        if storage in self.storage_to_limits:
            return self.storage_to_limits[storage].outbound_max_active
        if "*" in self.storage_to_limits:
            return self.storage_to_limits["*"].outbound_max_active
        raise Exception(
            "StorageLimits.get_storage_outbound_max_active(): "
            f"No storage-limit configuration: storage={storage} or storage=*"
        )


@dataclass
class LinkLimits:
    """
    Configured link limits. The main purpose of this class is to provide the following
    convenience method which encapsulates the concept of the default link from "*" to "*"
    """

    link_key_to_max_active: dict[tuple[str, str], int]

    def get_max_active(self, link_key: tuple[str, str]) -> int:
        """
        Returns the configured maximum number of concurrent transfers allowed on the specified link
        """
        max_active = 0
        if link_key in self.link_key_to_max_active:
            max_active = self.link_key_to_max_active[link_key]
        elif ("*", "*") in self.link_key_to_max_active:
            max_active = self.link_key_to_max_active[("*", "*")]
        else:
            raise Exception(
                "LinkLimits.get_link_max_active(): No link configuration: "
                "(source_se={source_se},dest_se={dest_se}) or (source_se=*, dest_se=*)"
            )

        return max_active


@dataclass
class SchedulerInput:  # pylint:disable=too-many-instance-attributes
    """
    The input to a single call to a scheduling algorithm
    """

    opaque_data: Any
    max_url_copy_processes: int
    queues: dict
    link_limits: LinkLimits
    active_stats: []
    optimizer_limits: dict
    storage_limits: StorageLimits
    vo_activity_shares: dict
    link_vo_shares: dict


class SchedulerOutput:
    """
    The output of a single call to a scheduling algorithm
    """

    def __init__(self):
        self._transfers_per_queue = {}
        self._total_nb_transfers = 0
        self._opaque_data = (
            None  # # Opaque data passed from one round of scheduling to the next
        )

    def __repr__(self):
        return (
            "SchedulerOutput("
            + f"transfers_per_queue={self._transfers_per_queue},"
            + f"total_nb_transfers={self._total_nb_transfers},"
            + f"opaque_data={self._opaque_data}"
            + ")"
        )

    def inc_transfers_for_queue(self, queue_id, nb_transfers):
        """
        Increments the number of file-transfers that should be scheduled for the given queue.  This
        method will create a new queue entry if nedded and will also keep the total number of
        transfers up-to-date.
        """
        self._transfers_per_queue[queue_id] = (
            self._transfers_per_queue[queue_id] + nb_transfers
            if queue_id in self._transfers_per_queue
            else nb_transfers
        )
        self._total_nb_transfers += nb_transfers

    def get_transfers_per_queue(self):
        """
        Returns the dictionary from queue ID to the number of file-transfers that should be
        scheduled
        """
        return self._transfers_per_queue

    def get_total_nb_transfers(self):
        """
        Returns the total number of file-transfers that should eb scheduled
        """
        return self._total_nb_transfers

    def set_opaque_data(self, opaque_data):
        """
        Sets the opaque data to be passed to the next call of the scheduler algorithm.  This
        opaque data can be used to treat queues fairly by not always starting with the same
        queues but to gradually interate over all of the queues over multiple calls to the
        scheduler algorithm.
        """
        self._opaque_data = opaque_data

    def get_opaque_data(self):
        """
        Returns the opaque data from the previous call to the scheduler algorithm.
        """
        return self._opaque_data


class SchedulerAlgo(ABC):  # pylint:disable=too-few-public-methods
    """
    Defines the interface to a object implementing a scheduler algorithm
    """

    def __init__(self, sched_input):
        self.sched_input = sched_input

    @abstractmethod
    def schedule(self) -> SchedulerOutput:
        """
        The entry point to the scheduler algorithm.
        """
