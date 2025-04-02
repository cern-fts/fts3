"""
Default file-transfer scheduling algorithm
"""

# pylint:disable=too-many-lines

from typing import Any
from dataclasses import dataclass
from scheduler_algo import SchedulerAlgo, SchedulerOutput


class RRSException(Exception):
    """
    Round-robin scheduler exception
    """


class RRS:
    """
    Round-Robin Scheduler (RSS)
    """

    def __init__(self):
        self._buf = []
        self._next_idx = 0

    def append(self, value):
        """
        Appends the specified value to the end of the round-robin scheduler
        """
        self._buf.append(value)

    def get_next(self):
        """
        Returns the next value
        """
        if not self._buf:
            raise RRSException("get_next(): Empty buffer")
        next_value = self._buf[self._next_idx]
        self._next_idx = (self._next_idx + 1) % len(self._buf)
        return next_value

    def __len__(self):
        return len(self._buf)

    def __bool__(self):
        return bool(self._buf)

    def __repr__(self):
        return (
            "RRS("
            + f"buf={self._buf},"
            + f"next_idx={self._next_idx},"
            + f"next={self._buf[self._next_idx] if self._buf else None}"
            ")"
        )

    def __contains__(self, item):
        return item in self._buf

    def skip_until_after(self, val_to_skip_over):
        """
        Skip through this round-robin scheduler until after the specified value
        """
        if not self._buf:
            return

        for idx, val in enumerate(self._buf):
            if val > val_to_skip_over:
                self._next_idx = idx
                return
        self._next_idx = 0

    def remove_value(self, value):
        """
        Removes the specified value from this round-robin scheduler
        """
        self._buf.remove(value)

        # Wrap next_idx around to 0 if has fallen off the buffer
        if self._next_idx == len(self._buf):
            self._next_idx = 0


@dataclass
class WRRSQ:
    """
    A queue within a Weighted Round-Robin Sheduler (WRRS)
    """

    q_id: Any
    weight: float  # Weight of this queue
    queued: int  # Number of queued "tasks"
    active: int  # Number of currently active "tasks"


class WRRSException(Exception):
    """
    Weighted Round-Robin Scheduler (WRRS) exception
    """


class WRRS:
    """
    An interleaved Weight Round-Robin Scheduler (WRRS)
    """

    def __init__(self, max_active: int, queues: list[WRRSQ]):
        self._max_active = max_active
        self._total_active = sum(q.active for q in queues)
        self._total_weight = sum(
            q.weight for q in queues if q.queued > 0
        )  # Ignore empty queues
        self._next_idx = 0
        self._queues = [q for q in queues if q.queued > 0]  # Ignore empty queues

    def next_queue_id(self):
        """
        Returns the next queue ID
        """
        # Return None if nothing queued or maximum activity reached
        if not self._queues or self._total_active >= self._max_active:
            return None

        queue = self._queues[self._next_idx]

        # Note:
        #
        # A dormant queue can become active in a later scheduler round.
        #
        # A queue is dormant if it has at least one queued "task" and has reached its target number
        # of active jobs.
        #
        # A dormant queue will become active if its target number of active jobs is increased.  This
        # can happen when a competing queue becomes empty and is removed.

        # Find the next active-queue
        found_active_queue = False
        for _ in range(len(self._queues)):
            queue = self._queues[self._next_idx]
            target_active = queue.weight / self._total_weight * self._max_active
            if target_active > 0:
                found_active_queue = True
                break
            # Skip dormant queue
            self._next_idx = (self._next_idx + 1) % len(self._queues)

        if not found_active_queue:
            raise WRRSException(
                f"next_queue_id(): Failed to find an active queue: {self}"
            )

        # Update queue counts
        queue.queued -= 1
        queue.active += 1
        self._total_active += 1

        # If the queue is now empty
        if queue.queued == 0:
            # Remove the queue and its weight from the next round
            del self._queues[self._next_idx]
            self._total_weight -= queue.weight

            # Wrap next_idx around to 0 if has fallen off the buffer
            self._next_idx = (
                0 if self._next_idx == len(self._queues) else self._next_idx
            )
        else:
            # Move to the next queue because this is an interleaved WRR
            self._next_idx = (self._next_idx + 1) % len(self._queues)

        return queue.q_id

    def __repr__(self):
        return (
            "WRRS("
            f"max_active={self._max_active},"
            f"total_active={self._total_active},"
            f"total_weight={self._total_weight},"
            f"next_idx={self._next_idx},"
            f"queues={self._queues}"
            ")"
        )

    def skip_until_after(self, q_id_to_skip_over):
        """
        Skip through this Weight Round-Robin scheduler until after the specified queue ID
        """
        if not self._queues:
            raise WRRSException("skip_until_after(): No queues")

        for idx, queue in enumerate(self._queues):
            if queue.q_id > q_id_to_skip_over:
                self._next_idx = idx
                return
        self._next_idx = 0

    def remove_queue(self, q_id):
        """
        Removes the queue with the specified ID
        """
        idx_to_del = None
        for idx, queue in enumerate(self._queues):
            if queue.q_id == q_id:
                idx_to_del = idx
                break

        if idx_to_del is None:
            return

        del self._queues[idx_to_del]

        # Wrap next_idx around to 0 if has fallen off the buffer
        self._next_idx = 0 if self._next_idx == len(self._queues) else self._next_idx


class TransferPotentialException(Exception):
    """
    Transfer-potential exception
    """


class TransferPotential:
    """
    The potential of a link or storage endpoint to carry out new file-transfers
    """

    def __init__(self, max_active: int, nb_active: int, nb_queued: int):
        self._max_active = max_active
        self._nb_active = nb_active
        self._nb_queued = nb_queued
        self._calc_potential()

    def __repr__(self):
        return (
            "TransferPotential("
            f"max_active={self._max_active},"
            f"nb_active={self._nb_active},"
            f"nb_queued={self._nb_queued},"
            f"potential={self._potential}"
            ")"
        )

    def _calc_potential(self):
        """
        Calculates the number of file-transfers that could potentially be scheduled
        """
        self._potential = min(
            self._nb_queued, max(0, self._max_active - self._nb_active)
        )

    def get_max_active(self) -> int:
        """
        Returns the maximum number of active file-transfers
        """
        return self._max_active

    def get_nb_active(self) -> int:
        """
        Returns the number of active file-tranfers
        """
        return self._nb_active

    def get_nb_queued(self) -> int:
        """
        Returns the number of queued file-transfers
        """
        return self._nb_queued

    def get_potential(self) -> int:
        """
        Returns the number of file-transfers that could potentially be scheduled
        """
        return self._potential

    def scheduled(self, nb_scheduled: int):
        """
        Updates the transfer-potential by taking into account the specified number of scheduled
        file-transfers
        """
        if nb_scheduled > self._nb_queued:
            raise TransferPotentialException(
                "TransferPotential.update(): nb_scheduled > nb_queued: "
                f"nb_scheduled={nb_scheduled} nb_queued={self._nb_queued}"
            )
        if self._nb_active + nb_scheduled > self._max_active:
            raise TransferPotentialException(
                "TransferPotential.update(): nb_active + nb_scheduled > max_active: "
                f"nb_active={self._nb_active} "
                f"nb_scheduled={nb_scheduled} "
                f"max_active={self._max_active}"
            )
        self._nb_queued -= nb_scheduled
        self._nb_active += nb_scheduled
        self._calc_potential()

    def set_max_active(self, max_active):
        """
        Updates the transfer-potential by taking into account the specified maximum number of
        active file-transfers
        """
        self._max_active = max_active
        self._calc_potential()


class PotentialLinksException(Exception):
    """
    Potential-links exception
    """


class PotentialLinks:
    """
    A Round-Robin Scheduler (RSS) of file-transfer links that have the potential for one or more
    file-transfers
    """

    def __init__(self, sched_input):
        self._sched_input = sched_input
        self._storage_to_outbound_potential = self._get_storage_to_outbound_potential()
        self._storage_to_inbound_potential = self._get_storage_to_inbound_potential()
        self._link_key_to_potential = self._get_link_key_to_potential(
            self._storage_to_outbound_potential, self._storage_to_inbound_potential
        )
        self.link_key_rrs = RRS()

        for link_key in sorted(self._link_key_to_potential.keys()):
            self.link_key_rrs.append(link_key)

    def __bool__(self):
        return bool(self.link_key_rrs)

    def skip_until_after(self, link_key):
        """
        Skip through this Weight Round-Robin scheduler until after the specified link
        """
        self.link_key_rrs.skip_until_after(link_key)

    def get_link_keys_with_potential(self):
        """
        Returns a list of the links with the potential to transfer at least one file
        """
        return list(self._link_key_to_potential.keys())

    def get_link_potential(self, link_key) -> TransferPotential:
        """
        Returns the potential of the specified link
        """
        return self._link_key_to_potential[link_key]

    def get_next(self):
        """
        Returns the next link to be scheduled
        """
        next_link_key = self.link_key_rrs.get_next()

        # Update storage and link potentials
        self._storage_to_outbound_potential[next_link_key[0]].scheduled(1)
        self._storage_to_inbound_potential[next_link_key[1]].scheduled(1)
        self._update_link_potential(
            next_link_key
        )  # Links must be updated AFTER storages

        # Remove staturated storages
        if not self._storage_to_outbound_potential[next_link_key[0]].get_potential():
            del self._storage_to_outbound_potential[next_link_key[0]]
        if not self._storage_to_inbound_potential[next_link_key[1]].get_potential():
            del self._storage_to_inbound_potential[next_link_key[1]]

        # Remove saturated links
        staturated_links = [
            link_key
            for link_key, potential in self._link_key_to_potential.items()
            if potential.get_potential() == 0
        ]
        for staturated_link_key in staturated_links:
            if staturated_link_key in self.link_key_rrs:
                self.link_key_rrs.remove_value(staturated_link_key)

        return next_link_key

    def _update_link_potential(self, next_link_key):
        """
        Update link potentials.  This method assumes storage potentials have already been updated.
        """
        self._link_key_to_potential[next_link_key].scheduled(1)

        # Apply storage potential updates to link potentials
        next_source_se = next_link_key[0]
        next_dest_se = next_link_key[1]
        for link_key, link_potential in self._link_key_to_potential.items():
            # Ignore link if not involved
            if link_key[0] != next_source_se and link_key[1] != next_dest_se:
                continue
            link_config_max_active = self._sched_input.link_limits.get_max_active(
                link_key
            )
            link_potential_max_active = min(
                link_config_max_active,
                self._storage_to_outbound_potential[next_source_se].get_potential(),
                self._storage_to_inbound_potential[next_dest_se].get_potential(),
            )
            link_optimizer_limit = self._get_link_optimizer_limit(next_link_key)
            link_potential_max_active = (
                min(link_potential_max_active, link_optimizer_limit)
                if link_optimizer_limit is not None
                else link_potential_max_active
            )
            link_potential.set_max_active(link_potential_max_active)

    def _get_link_key_to_potential(
        self, storage_to_outbound_potential, storage_to_inbound_potential
    ):
        """
        Returns a map from link to the number of transfers that could potentially be scheduled on
        that link.  The map only contains links that have at least 1 potential transfer.
        """
        link_key_to_nb_queued = self._get_link_key_to_nb_queued()

        link_key_to_potential = {}
        for link_key, nb_queued in link_key_to_nb_queued.items():
            link_potential = self._get_link_potential(
                link_key,
                nb_queued,
                storage_to_outbound_potential,
                storage_to_inbound_potential,
            )
            if link_potential.get_potential() > 0:
                link_key_to_potential[link_key] = link_potential
        return link_key_to_potential

    def _get_link_key_to_nb_queued(self):
        link_to_nb_queued = {}
        for queue in self._sched_input.queues.values():
            if queue.link_key not in link_to_nb_queued:
                link_to_nb_queued[queue.link_key] = queue.nb_queued
            else:
                link_to_nb_queued[queue.link_key] += queue.nb_queued
        return link_to_nb_queued

    def _get_link_potential(
        self,
        link_key,
        link_nb_queued,
        storage_to_outbound_potential,
        storage_to_inbound_potential,
    ):
        """
        Returns the number of transfers that could potentially be scheduled on the specified link.
        """
        source_se = link_key[0]
        dest_se = link_key[1]

        link_config_max_active = self._sched_input.link_limits.get_max_active(link_key)
        source_out_potential = storage_to_outbound_potential[source_se]
        dest_in_potential = storage_to_inbound_potential[dest_se]
        max_active = min(
            link_config_max_active,
            source_out_potential.get_potential(),
            dest_in_potential.get_potential(),
        )
        link_optimizer_limit = self._get_link_optimizer_limit(link_key)
        if link_optimizer_limit is not None:
            max_active = min(max_active, link_optimizer_limit)

        link_nb_active = self._get_link_nb_active(link_key)
        link_potential = TransferPotential(
            max_active=max_active, nb_active=link_nb_active, nb_queued=link_nb_queued
        )
        return link_potential

    def _get_link_optimizer_limit(self, link_key):
        return (
            self._sched_input.optimizer_limits[link_key]["active"]
            if link_key in self._sched_input.optimizer_limits
            else None
        )

    def _get_link_nb_active(self, link_key):
        result = 0
        for stats in self._sched_input.active_stats:
            source_se = stats["source_se"]
            dest_se = stats["dest_se"]
            nb_active = stats["nb_active"]
            stats_link_key = (source_se, dest_se)
            result += nb_active if link_key == stats_link_key else 0
        return result

    def _get_storage_to_outbound_to_nb_queued(self):
        result = {}
        for queue in self._sched_input.queues.values():
            queue_source_se = queue.link_key[0]  # link_key = (source_se, dest_se)
            if queue_source_se not in result:
                result[queue_source_se] = 0
            result[queue_source_se] += queue.nb_queued
        return result

    def _get_storage_to_inbound_to_nb_queued(self):
        result = {}
        for queue in self._sched_input.queues.values():
            queue_dest_se = queue.link_key[1]  # link_key = (source_se, dest_se)
            if queue_dest_se not in result:
                result[queue_dest_se] = 0
            result[queue_dest_se] += queue.nb_queued
        return result

    def _get_storage_to_outbound_potential(self):
        storages_with_outbound_queues = self._get_storages_with_outbound_queues()
        storage_to_outbound_active = self._get_storage_to_outbound_active()
        storage_to_outbound_nb_queued = self._get_storage_to_outbound_to_nb_queued()
        result = {}
        for storage in storages_with_outbound_queues:
            max_active = self._sched_input.storage_limits.get_outbound_max_active(
                storage
            )
            nb_active = (
                0
                if storage not in storage_to_outbound_active
                else storage_to_outbound_active[storage]
            )
            nb_queued = (
                0
                if storage not in storage_to_outbound_nb_queued
                else storage_to_outbound_nb_queued[storage]
            )

            result[storage] = TransferPotential(
                max_active=max_active, nb_active=nb_active, nb_queued=nb_queued
            )
        return result

    def _get_storage_to_inbound_potential(self):
        storages_with_inbound_queues = self._get_storages_with_inbound_queues()
        storage_to_inbound_active = self._get_storage_to_inbound_active()
        storage_to_inbound_nb_queued = self._get_storage_to_inbound_to_nb_queued()
        result = {}
        for storage in storages_with_inbound_queues:
            max_active = self._sched_input.storage_limits.get_inbound_max_active(
                storage
            )
            nb_active = (
                0
                if storage not in storage_to_inbound_active
                else storage_to_inbound_active[storage]
            )
            nb_queued = (
                0
                if storage not in storage_to_inbound_nb_queued
                else storage_to_inbound_nb_queued[storage]
            )

            result[storage] = TransferPotential(
                max_active=max_active, nb_active=nb_active, nb_queued=nb_queued
            )
        return result

    def _get_storages_with_outbound_queues(self):
        result = set()
        for queue in self._sched_input.queues.values():
            result.add(queue.link_key[0])  # link_key = (source_se, dest_se)
        return result

    def _get_storages_with_inbound_queues(self):
        result = set()
        for queue in self._sched_input.queues.values():
            result.add(queue.link_key[1])  # link_key = (source_se, dest_se)
        return result

    def _get_storage_to_outbound_active(self):
        result = {}
        for stats in self._sched_input.active_stats:
            storage = stats["source_se"]
            nb_active = stats["nb_active"]
            result[storage] = (
                0 if storage not in result else result[storage] + nb_active
            )
        return result

    def _get_storage_to_inbound_active(self):
        result = {}
        for stats in self._sched_input.active_stats:
            storage = stats["dest_se"]
            nb_active = stats["nb_active"]
            result[storage] = (
                0 if storage not in result else result[storage] + nb_active
            )
        return result


@dataclass
class PrevRun:
    """
    Opaque data passed from one round of scheduling to the next.  This data specifies the last
    scheduled link, VO and activity.
    """

    link_key: str
    vo_name: str
    activity: str


class SchedulingException(Exception):
    """
    Scheduling exception
    """


class DefaultSchedulerAlgo(SchedulerAlgo):  # pylint:disable=too-few-public-methods
    """
    The default file-transfer scheduling algorithm
    """

    def _get_sched_data(self) -> SchedulerOutput:
        """
        The entry point to the scheduler algorithm
        """
        sched_data = {}

        sched_data["potential_links"] = PotentialLinks(self.sched_input)
        sched_data["potential_concurrent_transfers"] = (
            self._get_potential_concurrent_transfers()
        )

        # Do nothing if:
        # - There is no queued work to be done OR
        # - Storages and links are saturated OR
        # - FTS is saturated (the FTS concurrent transfer limit has been reached)
        if (
            not self.sched_input.queues
            or not sched_data["potential_links"]
            or not sched_data["potential_concurrent_transfers"]
        ):
            return None

        link_to_vo_to_activity_to_queue = self._get_link_to_vo_to_activity_to_queue()

        potential_link_keys = sorted(
            sched_data["potential_links"].get_link_keys_with_potential()
        )

        # To be iteratively modified in order to know when to stop considering a VO
        sched_data["link_key_to_vo_to_nb_queued"] = (
            self._get_link_key_to_vo_to_nb_queued()
        )

        # To be iteratively modified in order to know when to stop considering an activity
        sched_data["link_key_to_vo_to_activity_to_nb_queued"] = (
            self._get_link_key_to_vo_to_activity_to_nb_queued()
        )

        # To be iteratively modified to respect activity shares
        sched_data["link_key_to_vo_to_activity_to_nb_active"] = (
            self._get_link_key_to_vo_to_activity_to_nb_active()
        )

        # Create link key -> VO round-robin scheduler
        sched_data["potential_link_to_vo_rrs"] = self._get_link_key_to_vo_rrs(
            potential_link_keys, link_to_vo_to_activity_to_queue
        )

        # Create link-key -> VO -> activity WRR
        sched_data["potential_link_to_vo_to_activity_wrr"] = (
            self._get_potential_link_to_vo_to_activity_wrr(
                sched_data["potential_links"],
                link_to_vo_to_activity_to_queue,
                sched_data["link_key_to_vo_to_activity_to_nb_queued"],
                sched_data["link_key_to_vo_to_activity_to_nb_active"],
            )
        )

        # Create link-key -> VO -> activity -> queue-id
        sched_data["potential_link_to_vo_to_activity_to_queue_id"] = {}
        for link_key in potential_link_keys:
            sched_data["potential_link_to_vo_to_activity_to_queue_id"][link_key] = {}
            for vo_name, activity_to_queue in link_to_vo_to_activity_to_queue[
                link_key
            ].items():
                sched_data["potential_link_to_vo_to_activity_to_queue_id"][link_key][
                    vo_name
                ] = {}
                for activity, queue in activity_to_queue.items():
                    sched_data["potential_link_to_vo_to_activity_to_queue_id"][
                        link_key
                    ][vo_name][activity] = queue.queue_id

        # Fast-forward schedulers based on previous scheduling run
        prev_run = self.sched_input.opaque_data
        if isinstance(prev_run, PrevRun):
            sched_data["potential_links"].skip_until_after(prev_run.link_key)
            if prev_run.link_key in sched_data["potential_link_to_vo_rrs"]:
                sched_data["potential_link_to_vo_rrs"][
                    prev_run.link_key
                ].skip_until_after(prev_run.vo_name)
            if prev_run.link_key in sched_data["potential_link_to_vo_to_activity_wrr"]:
                if (
                    prev_run.vo_name
                    in sched_data["potential_link_to_vo_to_activity_wrr"][
                        prev_run.link_key
                    ]
                ):
                    sched_data["potential_link_to_vo_to_activity_wrr"][
                        prev_run.link_key
                    ][prev_run.vo_name].skip_until_after(prev_run.activity)

        return sched_data

    def schedule(self) -> SchedulerOutput:
        """
        The entry point to the scheduler algorithm
        """
        sched_data = self._get_sched_data()
        if sched_data is None:
            return None

        # Round robin free work-capacity across submission queues taking into account any
        # constraints
        sched_output = SchedulerOutput()
        while (
            sched_data["potential_concurrent_transfers"]
            and sched_data["potential_links"]
        ):
            # Identify the link and storages that could do the work
            link_key = sched_data["potential_links"].get_next()
            source_se = link_key[0]
            dest_se = link_key[1]

            # Get the round-robin scheduler of VOs on the link
            vo_rrs = sched_data["potential_link_to_vo_rrs"][link_key]

            # Get the next VO of the link
            vo_name = vo_rrs.get_next()

            # Get the weighted round-robin scheduler of activities of the VO on the link
            activity_wrr = sched_data["potential_link_to_vo_to_activity_wrr"][link_key][
                vo_name
            ]

            # Get the next activity of the VO on the link
            activity = activity_wrr.next_queue_id()

            # Get the ID of the next eligble queue
            queue_id = sched_data["potential_link_to_vo_to_activity_to_queue_id"][
                link_key
            ][vo_name][activity]

            # Schedule a transfer for this queue
            sched_output.inc_transfers_for_queue(queue_id, 1)
            sched_data["potential_concurrent_transfers"] -= 1

            # Update active file-transfers in order to respect activit shares
            if link_key not in sched_data["link_key_to_vo_to_activity_to_nb_active"]:
                sched_data["link_key_to_vo_to_activity_to_nb_active"][link_key] = {}
            if (
                vo_name
                not in sched_data["link_key_to_vo_to_activity_to_nb_active"][link_key]
            ):
                sched_data["link_key_to_vo_to_activity_to_nb_active"][link_key][
                    vo_name
                ] = {}
            sched_data["link_key_to_vo_to_activity_to_nb_active"][link_key][vo_name][
                activity
            ] = (
                sched_data["link_key_to_vo_to_activity_to_nb_active"][link_key][
                    vo_name
                ][activity]
                + 1
                if activity
                in sched_data["link_key_to_vo_to_activity_to_nb_active"][link_key][
                    vo_name
                ]
                else 1
            )

            # Update the scheduling opaque-data
            sched_output.set_opaque_data(
                PrevRun(link_key=link_key, vo_name=vo_name, activity=activity)
            )

            # Remove VO from VO round-robin scheduler if necessary
            sched_data["link_key_to_vo_to_nb_queued"][link_key][vo_name] -= 1
            if sched_data["link_key_to_vo_to_nb_queued"][link_key][vo_name] < 0:
                raise SchedulingException(
                    "Link to VO to nb_queued went negative: "
                    f"source_se={source_se} dest_se={dest_se} vo_name={vo_name}"
                )
            if sched_data["link_key_to_vo_to_nb_queued"][link_key][vo_name] == 0:
                vo_rrs.remove_value(vo_name)

            # Remove activity from activty round-robin scheduler if necessary
            sched_data["link_key_to_vo_to_activity_to_nb_queued"][link_key][vo_name][
                activity
            ] -= 1
            if (
                sched_data["link_key_to_vo_to_activity_to_nb_queued"][link_key][
                    vo_name
                ][activity]
                < 0
            ):
                raise SchedulingException(
                    "Link to VO to activity to nb_queued went negative: "
                    f"source_se={source_se} dest_se={dest_se} vo_name={vo_name} activity={activity}"
                )
            if (
                sched_data["link_key_to_vo_to_activity_to_nb_queued"][link_key][
                    vo_name
                ][activity]
                == 0
            ):
                activity_wrr.remove_queue(activity)

        return sched_output

    def _get_link_key_to_vo_to_nb_queued(self):
        result = {}
        for queue in self.sched_input.queues.values():
            link_key = queue.link_key
            vo_name = queue.vo_name
            nb_queued = queue.nb_queued

            if link_key not in result:
                result[link_key] = {}
            result[link_key][vo_name] = (
                nb_queued
                if vo_name not in result[link_key]
                else result[link_key][vo_name] + nb_queued
            )
        return result

    def _get_vo_activities_of_queues(self, queue_ids):
        vo_activities = {}  # Activities are grouped by VO
        for queue_id in queue_ids:
            queue = self.sched_input.queues[queue_id]
            vo_name = queue["vo_name"]
            activity = queue["activity"]
            if vo_name not in vo_activities:
                vo_activities[vo_name] = []
            vo_activities[vo_name].append(activity)
        return vo_activities

    def _get_link_key_to_queues(self):
        link_key_to_queues = {}
        for queue_id, queue in self.sched_input.queues.items():
            if queue.link_key not in link_key_to_queues:
                link_key_to_queues[queue.link_key] = {}
            link_key_to_queues[queue.link_key][queue_id] = queue
        return link_key_to_queues

    def _get_link_to_vo_to_activity_to_queue(self):
        result = {}
        for queue in self.sched_input.queues.values():
            if queue.link_key not in result:
                result[queue.link_key] = {}
            if queue.vo_name not in result[queue.link_key]:
                result[queue.link_key][queue.vo_name] = {}
            result[queue.link_key][queue.vo_name][queue.activity] = queue
        return result

    def _get_link_key_to_vo_to_activity_to_nb_queued(self):
        result = {}
        for queue in self.sched_input.queues.values():
            if queue.link_key not in result:
                result[queue.link_key] = {}
            if queue.vo_name not in result[queue.link_key]:
                result[queue.link_key][queue.vo_name] = {}
            result[queue.link_key][queue.vo_name][queue.activity] = queue.nb_queued
        return result

    def _get_link_optimizer_limit(self, link_key):
        return (
            self.sched_input.optimizer_limits[link_key]["active"]
            if link_key in self.sched_input.optimizer_limits
            else None
        )

    def _get_link_key_to_vo_to_activity_to_nb_active(self):
        result = {}
        for stats in self.sched_input.active_stats:
            source_se = stats["source_se"]
            dest_se = stats["dest_se"]
            link_key = (source_se, dest_se)
            vo_name = stats["vo_name"]
            activity = stats["activity"]
            nb_active = stats["nb_active"]

            if link_key not in result:
                result[link_key] = {}
            if vo_name not in result[link_key]:
                result[link_key][vo_name] = {}
            result[link_key][vo_name][activity] = nb_active
        return result

    @staticmethod
    def _get_link_key_to_vo_rrs(potential_link_keys, link_to_vo_to_activity_to_queue):
        result = {}
        for link_key in potential_link_keys:
            vo_rrs = RRS()
            for vo_name in sorted(link_to_vo_to_activity_to_queue[link_key].keys()):
                vo_rrs.append(vo_name)
            result[link_key] = vo_rrs
        return result

    def _get_link_nb_active_per_vo(self, link_key):
        nb_active_per_vo = {}
        for queue in self.sched_input.queues.values():
            queue_link_key = (queue["source_se"], queue["dest_se"])
            if queue_link_key == link_key:
                vo_name = queue["vo"]
                nb_files = queue["nb_files"]

                if vo_name not in nb_active_per_vo:
                    nb_active_per_vo[vo_name] = 0
                nb_active_per_vo[vo_name] += nb_files
        return nb_active_per_vo

    def _get_link_nb_active_per_vo_activity(self, link_key):
        nb_active_per_vo_activity = {}
        for queue in self.sched_input.queues.values():
            queue_link_key = (queue["source_se"], queue["dest_se"])
            if queue_link_key == link_key:
                vo_name = queue["vo"]
                activity = queue["activity"]
                nb_files = queue["nb_files"]

                if vo_name not in nb_active_per_vo_activity:
                    nb_active_per_vo_activity[vo_name] = {}
                if activity not in nb_active_per_vo_activity[vo_name]:
                    nb_active_per_vo_activity[vo_name][activity] = 0
                nb_active_per_vo_activity[vo_name][activity] += nb_files
        return nb_active_per_vo_activity

    def _get_potential_concurrent_transfers(self):
        """
        Returns the number of potential concurrent-transfers
        """
        nb_active = 0
        for stats in self.sched_input.active_stats:
            nb_active += stats["nb_active"]
        return max(0, self.sched_input.max_url_copy_processes - nb_active)

    def _get_potential_link_to_vo_to_activity_wrr(
        self,
        potential_links,
        link_to_vo_to_activity_to_queue,
        link_key_to_vo_to_activity_to_nb_queued,
        link_key_to_vo_to_activity_to_nb_active,
    ):
        result = {}
        for link_key in sorted(potential_links.get_link_keys_with_potential()):
            result[link_key] = {}
            for vo_name in link_to_vo_to_activity_to_queue[link_key].keys():
                activity_shares = (
                    self.sched_input.vo_activity_shares[vo_name]
                    if vo_name in self.sched_input.vo_activity_shares
                    else {"default": 1}
                )
                if "default" not in activity_shares:
                    raise SchedulingException(
                        f"Default activity share missing from VO activity-shares: vo_name={vo_name}"
                    )

                self._spread_default_weight_over_weightless_activities(
                    link_key_to_vo_to_activity_to_nb_queued[link_key][vo_name].keys(),
                    activity_shares,
                )
                activity_queues = []
                for activity, weight in activity_shares.items():
                    queued = (
                        link_key_to_vo_to_activity_to_nb_queued[link_key][vo_name][
                            activity
                        ]
                        if link_key in link_key_to_vo_to_activity_to_nb_queued
                        and vo_name in link_key_to_vo_to_activity_to_nb_queued[link_key]
                        and activity
                        in link_key_to_vo_to_activity_to_nb_queued[link_key][vo_name]
                        else 0
                    )
                    active = (
                        link_key_to_vo_to_activity_to_nb_active[link_key][vo_name][
                            activity
                        ]
                        if link_key in link_key_to_vo_to_activity_to_nb_active
                        and vo_name in link_key_to_vo_to_activity_to_nb_active[link_key]
                        and activity
                        in link_key_to_vo_to_activity_to_nb_active[link_key][vo_name]
                        else 0
                    )

                    activity_queues.append(
                        WRRSQ(
                            q_id=activity, weight=weight, queued=queued, active=active
                        )
                    )
                result[link_key][vo_name] = WRRS(
                    max_active=potential_links.get_link_potential(
                        link_key
                    ).get_max_active(),
                    queues=activity_queues,
                )
        return result

    @staticmethod
    def _spread_default_weight_over_weightless_activities(
        queued_activities, activity_shares
    ):
        """
        Modifies the specified activity shares by equally spreading the weight of the default
        activity over the default activity itself and activities that have no configured weight
        """
        weightless_activities = {
            activity
            for activity in queued_activities
            if activity not in activity_shares
        }
        if weightless_activities:
            nb_weightless_activities_and_default = len(weightless_activities) + 1
            total_default_weight = activity_shares["default"]
            split_default_weight = (
                total_default_weight / nb_weightless_activities_and_default
            )
            activity_shares["default"] = split_default_weight
            for weightless_activity in weightless_activities:
                activity_shares[weightless_activity] = split_default_weight
