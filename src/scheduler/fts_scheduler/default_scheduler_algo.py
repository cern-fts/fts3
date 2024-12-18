"""
Default file-transfer scheduling algorithm
"""

from typing import Any
from dataclasses import dataclass
from scheduler_algo import SchedulerAlgo, SchedulerOutput


class CircularBuffer:
    """
    A circular buffer
    """

    def __init__(self):
        self._buf = []
        self._next_idx = 0

    def append(self, value):
        """
        Appends the specified value to the end of teh circular buffer
        """
        self._buf.append(value)

    def get_next(self):
        """
        Returns the next value in the circular buffer and advances to the next
        """
        if not self._buf:
            raise Exception("get_next(): Empty buffer")
        next_value = self._buf[self._next_idx]
        self._next_idx = (self._next_idx + 1) % len(self._buf)
        return next_value

    def __len__(self):
        return len(self._buf)

    def __bool__(self):
        return bool(self._buf)

    def __repr__(self):
        return (
            "CircularBuffer("
            + f"buf={self._buf},"
            + f"next_idx={self._next_idx},"
            + f"next={self._buf[self._next_idx] if self._buf else None}"
            ")"
        )

    def __contains__(self, item):
        return item in self._buf

    def skip_until_after(self, val_to_skip_over):
        """
        Skip through this circular-buffer until after the specified value
        """
        if not self._buf:
            raise Exception("skip_until_after(): Circular-buffer is empty")

        for idx, val in enumerate(self._buf):
            if val > val_to_skip_over:
                self._next_idx = idx
                return
        self._next_idx = 0

    def remove_value(self, value):
        """
        Removes the specified value from the circular buffer
        """
        try:
            self._buf.remove(value)
        except Exception as ex:
            raise Exception(f"remove_value(): value={value}: {ex}") from ex

        # Wrap next_idx around to 0 if has fallen off the buffer
        if self._next_idx == len(self._buf):
            self._next_idx = 0


@dataclass
class WRRQ:
    """
    A queue within a Weighted Round-Robin (WRR) scheduler
    """

    q_id: Any
    weight: float
    queued: int
    active: int


class WRR:
    """
    An interleaved Weight Round-Robin (WRR) scheduler
    """

    def __init__(self, max_active: int, queues: list[WRRQ]):
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

        # A queue is dormant if it has queued work and has reached its target number of active jobs.
        # A dormant queue can become active in a later scheduler round because the target number of
        # active jobs will increase when and empty queue is removed.

        # Find the next active-queue
        found_active_queue = False
        for _ in range(len(self._queues)):
            queue = self._queues[self._next_idx]
            target = round(queue.weight / self._total_weight * self._max_active)
            queue_is_active = queue.active < target
            if queue_is_active:
                found_active_queue = True
                break
            # Skip dormant queue
            self._next_idx = (self._next_idx + 1) % len(self._queues)
        if not found_active_queue:
            raise Exception(f"next_queue_id(): Failed to find an active queue: {self}")

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
            "WRR("
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
            raise Exception("skip_until_after(): No queues")

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


class LinkPotential:
    """
    The potential of a link
    """

    def __init__(self, max_active: int, nb_active: int, nb_queued: int):
        self._max_active = max_active
        self._nb_active = nb_active
        self._nb_queued = nb_queued
        self._calc_potential()

    def __repr__(self):
        return (
            "LinkPotential("
            f"max_active={self._max_active},"
            f"nb_active={self._nb_active},"
            f"nb_queued={self._nb_queued},"
            f"potential={self._potential}"
            ")"
        )

    def _calc_potential(self):
        """
        Calculates the number of file-transfers that could porentially be scheduled
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
        Returns the number of file-transfers that could porentially be scheduled
        """
        return self._potential

    def scheduled(self, nb_scheduled: int):
        """
        Updates the potential of the link taking into account the specified number of scheduled
        file-transfers
        """
        if nb_scheduled > self._nb_queued:
            raise Exception(
                "LinkPotential.update(): nb_scheduled > nb_queued: "
                f"nb_scheduled={nb_scheduled} nb_queued={self._nb_queued}"
            )
        if self._nb_active + nb_scheduled > self._max_active:
            raise Exception(
                "LinkPotential.update(): nb_active + nb_scheduled > max_active:"
                f"nb_active={self._nb_active} "
                f"nb_scheduled={nb_scheduled} "
                f"max_active={self._max_active}"
            )
        self._nb_queued -= nb_scheduled
        self._nb_active += nb_scheduled
        self._calc_potential()

    def set_max_active(self, max_active):
        """
        Updates the potential of the link taking into account the specified maximum number of active
        file-transfers
        """
        self._max_active = max_active
        self._calc_potential()


class DefaultSchedulerAlgo(SchedulerAlgo):  # pylint:disable=too-few-public-methods
    """
    The default file-transfer scheduling algorithm
    """

    def schedule(self) -> SchedulerOutput:
        """
        The entry point to the scheduler algorithm
        """
        potential_concurrent_transfers = self._get_potential_concurrent_transfers()

        # Do nothing if no work to be done or concurrent transfer limit reached
        if not self.sched_input["queues"] or not potential_concurrent_transfers:
            return None

        link_key_to_potential = self._get_link_key_to_potential()

        # Do nothing if there are no links with the potential to schedule a transfer
        if not link_key_to_potential:
            return None

        storage_to_outbound_potential = self._get_storage_to_outbound_potential()
        storage_to_inbound_potential = self._get_storage_to_inbound_potential()

        link_to_vo_to_activity_to_queue = self._get_link_to_vo_to_activity_to_queue()

        potential_link_keys = sorted(link_key_to_potential.keys())

        # To be iteratively modified in order to know when to stop considering a VO
        link_key_to_vo_to_nb_queued = self._get_link_key_to_vo_to_nb_queued()

        # To be iteratively modified in order to know when to stop considering an activity
        link_key_to_vo_to_activity_to_nb_queued = (
            self._get_link_key_to_vo_to_activity_to_nb_queued()
        )

        # To be iteratively modified to respect activity shares
        link_key_to_vo_to_activity_to_nb_active = (
            self._get_link_key_to_vo_to_activity_to_nb_active()
        )

        # Create a circular-buffer of the keys of links with the potential to schedule a transfer
        potential_link_key_cbuf = CircularBuffer()
        for link_key in potential_link_keys:
            potential_link_key_cbuf.append(link_key)

        # Create link key -> VO circular-buffer
        potential_link_to_vo_cbuf = {}
        for link_key in potential_link_keys:
            vo_cbuf = CircularBuffer()
            for vo_name in sorted(link_to_vo_to_activity_to_queue[link_key].keys()):
                vo_cbuf.append(vo_name)
            potential_link_to_vo_cbuf[link_key] = vo_cbuf

        # Create link-key -> VO -> activity WRR
        potential_link_to_vo_to_activity_wrr = {}
        for link_key in potential_link_keys:
            link_potential = link_key_to_potential[link_key]
            potential_link_to_vo_to_activity_wrr[link_key] = {}
            vos = link_to_vo_to_activity_to_queue[link_key].keys()
            for vo_name in vos:
                activity_shares = self.sched_input["vo_activity_shares"][vo_name]
                activity_queues = []
                for activity, weight in activity_shares.items():
                    activity_queued = 0
                    if (
                        link_key in link_key_to_vo_to_activity_to_nb_queued
                        and vo_name in link_key_to_vo_to_activity_to_nb_queued[link_key]
                        and activity
                        in link_key_to_vo_to_activity_to_nb_queued[link_key][vo_name]
                    ):
                        activity_queued = link_key_to_vo_to_activity_to_nb_queued[
                            link_key
                        ][vo_name][activity]
                    activity_active = 0
                    if (
                        link_key in link_key_to_vo_to_activity_to_nb_active
                        and vo_name in link_key_to_vo_to_activity_to_nb_active[link_key]
                        and activity
                        in link_key_to_vo_to_activity_to_nb_active[link_key][vo_name]
                    ):
                        activity_active = link_key_to_vo_to_activity_to_nb_active[
                            link_key
                        ][vo_name][activity]

                    activity_queue = WRRQ(
                        q_id=activity,
                        weight=weight,
                        queued=activity_queued,
                        active=activity_active,
                    )
                    activity_queues.append(activity_queue)
                activity_wrr = WRR(
                    max_active=link_potential.get_max_active(), queues=activity_queues
                )
                potential_link_to_vo_to_activity_wrr[link_key][vo_name] = activity_wrr

        # Create link-key -> VO -> activity -> queue-id
        potential_link_to_vo_to_activity_to_queue_id = {}
        for link_key in potential_link_keys:
            potential_link_to_vo_to_activity_to_queue_id[link_key] = {}
            for vo_name, activity_to_queue in link_to_vo_to_activity_to_queue[
                link_key
            ].items():
                potential_link_to_vo_to_activity_to_queue_id[link_key][vo_name] = {}
                for activity, queue in activity_to_queue.items():
                    queue_id = queue["queue_id"]
                    potential_link_to_vo_to_activity_to_queue_id[link_key][vo_name][
                        activity
                    ] = queue_id

        # Fast-forward circular buffers and WRR schedulers based on previous scheduling run
        if self.sched_input["opaque_data"]:
            sched_opaque_data = self.sched_input["opaque_data"]
            if "id_of_last_scheduled_link" in sched_opaque_data:
                potential_link_key_cbuf.skip_until_after(
                    sched_opaque_data["id_of_last_scheduled_link"]
                )
            if "link_to_last_scheduled_vo" in sched_opaque_data:
                for link_key, vo_name in sched_opaque_data[
                    "link_to_last_scheduled_vo"
                ].items():
                    if link_key in potential_link_to_vo_cbuf:
                        potential_link_to_vo_cbuf[link_key].skip_until_after(vo_name)
            if "link_to_vo_to_last_scheduled_activity" in sched_opaque_data:
                for link_key, vo_to_activity in sched_opaque_data[
                    "link_to_vo_to_last_scheduled_activity"
                ].items():
                    if link_key in potential_link_to_vo_to_activity_wrr:
                        for vo_name, activity in vo_to_activity.items():
                            if (
                                vo_name
                                in potential_link_to_vo_to_activity_wrr[link_key]
                            ):
                                potential_link_to_vo_to_activity_wrr[link_key][
                                    vo_name
                                ].skip_until_after(activity)

        # Round robin free work-capacity across submission queues taking into account any
        # constraints
        scheduler_decision = SchedulerOutput()
        while potential_concurrent_transfers and potential_link_key_cbuf:
            # Identify the link and storages that could do the work
            link_key = potential_link_key_cbuf.get_next()
            source_se = link_key[0]
            dest_se = link_key[1]

            # Get the circular buffer of VOs on the link
            vo_cbuf = potential_link_to_vo_cbuf[link_key]

            # Get the next VO of the link
            vo_name = vo_cbuf.get_next()

            # Get the WRR scheduler of activities of the VO on the link
            activity_wrr = potential_link_to_vo_to_activity_wrr[link_key][vo_name]

            # Get the next activity of the VO on the link
            activity = activity_wrr.next_queue_id()

            # Get the ID of the next eligble queue
            queue_id = potential_link_to_vo_to_activity_to_queue_id[link_key][vo_name][
                activity
            ]

            # Schedule a transfer for this queue
            scheduler_decision.inc_transfers_for_queue(queue_id, 1)
            potential_concurrent_transfers -= 1

            # Update active file-transfers in order to respect activit shares
            if link_key not in link_key_to_vo_to_activity_to_nb_active:
                link_key_to_vo_to_activity_to_nb_active[link_key] = {}
            if vo_name not in link_key_to_vo_to_activity_to_nb_active[link_key]:
                link_key_to_vo_to_activity_to_nb_active[link_key][vo_name] = {}
            link_key_to_vo_to_activity_to_nb_active[link_key][vo_name][activity] = (
                link_key_to_vo_to_activity_to_nb_active[link_key][vo_name][activity] + 1
                if activity
                in link_key_to_vo_to_activity_to_nb_active[link_key][vo_name]
                else 1
            )

            # Update the scheduling opaque-data
            if not scheduler_decision.get_opaque_data():
                scheduler_decision.set_opaque_data({})
                scheduler_decision.get_opaque_data()["link_to_last_scheduled_vo"] = {}
                scheduler_decision.get_opaque_data()[
                    "link_to_vo_to_last_scheduled_activity"
                ] = {}
            opaque_data = scheduler_decision.get_opaque_data()
            opaque_data["id_of_last_scheduled_link"] = link_key
            opaque_data["link_to_last_scheduled_vo"][link_key] = vo_name
            if link_key not in opaque_data["link_to_vo_to_last_scheduled_activity"]:
                opaque_data["link_to_vo_to_last_scheduled_activity"][link_key] = {}
            opaque_data["link_to_vo_to_last_scheduled_activity"][link_key][
                vo_name
            ] = activity

            # Update storages to reflect remaining work to be done
            saturated_storages = []
            storage_to_outbound_potential[source_se] -= 1
            if storage_to_outbound_potential[source_se] < 0:
                raise Exception(
                    f"Outbound potential of storage went negative: source_se={source_se}"
                )
            if storage_to_outbound_potential[source_se] == 0:
                saturated_storages.append(source_se)
            storage_to_inbound_potential[dest_se] -= 1
            if storage_to_inbound_potential[dest_se] < 0:
                raise Exception(
                    f"Inbound potential of storage went negative: dest_se={dest_se}"
                )
            if storage_to_inbound_potential[dest_se] == 0:
                saturated_storages.append(dest_se)

            # Update link potential to reflect remaining work to be done
            link_key_to_potential[link_key].scheduled(1)

            # Apply storage potential updates to link potentials
            for link_potential_key, link_potential in link_key_to_potential.items():
                if link_potential_key[0] != source_se and link_key[1] != dest_se:
                    break
                link_config_max_active = self._get_link_config_max_active(
                    link_potential_key
                )
                link_potential_max_active = min(
                    link_config_max_active,
                    storage_to_outbound_potential[source_se],
                    storage_to_inbound_potential[dest_se],
                )
                link_optimizer_limit = self._get_link_optimizer_limit(link_key)
                link_potential_max_active = (
                    min(link_potential_max_active, link_optimizer_limit)
                    if link_optimizer_limit is not None
                    else link_potential_max_active
                )
                link_potential.set_max_active(link_potential_max_active)

            # Remove saturated links from circular buffer
            staturated_links = [
                link_key
                for link_key, potential in link_key_to_potential.items()
                if potential.get_potential() == 0
            ]
            for staturated_link_key in staturated_links:
                if staturated_link_key in potential_link_key_cbuf:
                    potential_link_key_cbuf.remove_value(staturated_link_key)

            # Remove VO from VO circular-buffer if necessary
            link_key_to_vo_to_nb_queued[link_key][vo_name] -= 1
            if link_key_to_vo_to_nb_queued[link_key][vo_name] < 0:
                raise Exception(
                    "Link to VO to nb_queued went negative: "
                    f"source_se={source_se} dest_se={dest_se} vo_name={vo_name}"
                )
            if link_key_to_vo_to_nb_queued[link_key][vo_name] == 0:
                vo_cbuf.remove_value(vo_name)

            # Remove activity from activty circular-buffer if necessary
            link_key_to_vo_to_activity_to_nb_queued[link_key][vo_name][activity] -= 1
            if link_key_to_vo_to_activity_to_nb_queued[link_key][vo_name][activity] < 0:
                raise Exception(
                    "Link to VO to activity to nb_queued went negative: "
                    f"source_se={source_se} dest_se={dest_se} vo_name={vo_name} activity={activity}"
                )
            if (
                link_key_to_vo_to_activity_to_nb_queued[link_key][vo_name][activity]
                == 0
            ):
                activity_wrr.remove_queue(activity)

        return scheduler_decision

    def _get_link_key_to_vo_to_nb_queued(self):
        result = {}
        for queue in self.sched_input["queues"].values():
            vo_name = queue["vo_name"]
            source_se = queue["source_se"]
            dest_se = queue["dest_se"]
            link_key = (source_se, dest_se)
            nb_files = queue["nb_files"]

            if link_key not in result:
                result[link_key] = {}
            result[link_key][vo_name] = (
                nb_files
                if vo_name not in result[link_key]
                else result[link_key][vo_name] + nb_files
            )
        return result

    def _get_vo_activities_of_queues(self, queue_ids):
        vo_activities = {}  # Activities are grouped by VO
        for queue_id in queue_ids:
            queue = self.sched_input["queues"][queue_id]
            vo_name = queue["vo_name"]
            activity = queue["activity"]
            if vo_name not in vo_activities:
                vo_activities[vo_name] = []
            vo_activities[vo_name].append(activity)
        return vo_activities

    def _get_link_key_to_queues(self):
        link_key_to_queues = {}
        for queue_id, queue in self.sched_input["queues"].items():
            link_key = (queue["source_se"], queue["dest_se"])
            if link_key not in link_key_to_queues:
                link_key_to_queues[link_key] = {}
            link_key_to_queues[link_key][queue_id] = queue
        return link_key_to_queues

    def _get_link_to_vo_to_activity_to_queue(self):
        result = {}
        for queue in self.sched_input["queues"].values():
            link_key = (queue["source_se"], queue["dest_se"])
            vo_name = queue["vo_name"]
            activity = queue["activity"]

            if link_key not in result:
                result[link_key] = {}
            if vo_name not in result[link_key]:
                result[link_key][vo_name] = {}
            if activity not in result:
                result[activity] = {}
            result[link_key][vo_name][activity] = queue
        return result

    def _get_link_key_to_vo_to_activity_to_nb_queued(self):
        result = {}
        for queue in self.sched_input["queues"].values():
            link_key = (queue["source_se"], queue["dest_se"])
            vo_name = queue["vo_name"]
            activity = queue["activity"]
            nb_files = queue["nb_files"]

            if link_key not in result:
                result[link_key] = {}
            if vo_name not in result[link_key]:
                result[link_key][vo_name] = {}
            if activity not in result:
                result[activity] = {}
            result[link_key][vo_name][activity] = nb_files
        return result

    def _get_link_config_max_active(self, link_key):
        """
        Returns the configured maximum number of concurrent transfers allowed on the specified link
        """
        max_active = 0
        if link_key in self.sched_input["link_limits"]:
            max_active = self.sched_input["link_limits"][link_key]["max_active"]
        elif ("*", "*") in self.sched_input["link_limits"]:
            max_active = self.sched_input["link_limits"][("*", "*")]["max_active"]
        else:
            raise Exception(
                "DefaultSchedulerAlgo._get_link_max_active(): No link configuration found for "
                "(source_se={source_se},dest_se={dest_se}) or (source_se=*, dest_se=*)"
            )

        return max_active

    def _get_link_optimizer_limit(self, link_key):
        return (
            self.sched_input["optimizer_limits"][link_key]["active"]
            if link_key in self.sched_input["optimizer_limits"]
            else None
        )

    def _get_link_key_to_vo_to_activity_to_nb_active(self):
        result = {}
        for stats in self.sched_input["active_stats"]:
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

    def _get_storage_to_outbound_active(self):
        result = {}
        for stats in self.sched_input["active_stats"]:
            storage = stats["source_se"]
            nb_active = stats["nb_active"]
            result[storage] = (
                0 if storage not in result else result[storage] + nb_active
            )
        return result

    def _get_storage_to_inbound_active(self):
        result = {}
        for stats in self.sched_input["active_stats"]:
            storage = stats["dest_se"]
            nb_active = stats["nb_active"]
            result[storage] = (
                0 if storage not in result else result[storage] + nb_active
            )
        return result

    def _get_storages_with_outbound_queues(self):
        result = set()
        for queue in self.sched_input["queues"].values():
            result.add(queue["source_se"])
        return result

    def _get_storages_with_inbound_queues(self):
        result = set()
        for queue in self.sched_input["queues"].values():
            result.add(queue["dest_se"])
        return result

    def _get_storage_to_outbound_potential(self):
        storages_with_outbound_queues = self._get_storages_with_outbound_queues()
        storage_to_outbound_active = self._get_storage_to_outbound_active()
        result = {}
        for storage in storages_with_outbound_queues:
            max_active = self._get_storage_outbound_max_active(storage)
            nb_active = (
                0
                if storage not in storage_to_outbound_active
                else storage_to_outbound_active[storage]
            )
            result[storage] = 0 if nb_active >= max_active else max_active - nb_active
        return result

    def _get_storage_to_inbound_potential(self):
        storages_with_inbound_queues = self._get_storages_with_inbound_queues()
        storage_to_inbound_active = self._get_storage_to_inbound_active()
        result = {}
        for storage in storages_with_inbound_queues:
            max_active = self._get_storage_inbound_max_active(storage)
            nb_active = (
                0
                if storage not in storage_to_inbound_active
                else storage_to_inbound_active[storage]
            )
            result[storage] = 0 if nb_active >= max_active else max_active - nb_active
        return result

    def _get_storage_outbound_active(self, storage):
        nb_active = 0
        for stats in self.sched_input["active_stats"]:
            nb_active += stats["nb_active"] if storage == stats["source_se"] else 0
        return nb_active

    def _get_storage_inbound_active(self, storage):
        nb_active = 0
        for stats in self.sched_input["active_stats"]:
            nb_active += stats["nb_active"] if storage == stats["dest_se"] else 0
        return nb_active

    def _get_storage_outbound_potential(self, storage):
        active = self._get_storage_outbound_active(storage)
        max_active = self._get_storage_outbound_max_active(storage)
        return 0 if active >= max_active else max_active - active

    def _get_storage_inbound_potential(self, storage):
        active = self._get_storage_inbound_active(storage)
        max_active = self._get_storage_inbound_max_active(storage)
        return 0 if active >= max_active else max_active - active

    def _get_link_nb_active(self, link_key):
        result = 0
        for stats in self.sched_input["active_stats"]:
            source_se = stats["source_se"]
            dest_se = stats["dest_se"]
            nb_active = stats["nb_active"]
            stats_link_key = (source_se, dest_se)
            result += nb_active if link_key == stats_link_key else 0
        return result

    def _get_link_to_nb_queued(self):
        link_to_nb_queued = {}
        for queue in self.sched_input["queues"].values():
            link_key = (queue["source_se"], queue["dest_se"])
            if link_key not in link_to_nb_queued:
                link_to_nb_queued[link_key] = queue["nb_files"]
            else:
                link_to_nb_queued[link_key] += queue["nb_files"]
        return link_to_nb_queued

    def _get_link_nb_active_per_vo(self, link_key):
        nb_active_per_vo = {}
        for queue in self.sched_input["queues"].values():
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
        for queue in self.sched_input["queues"].values():
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

    def _get_link_potential(self, link_key, link_nb_queued):
        """
        Returns the number of transfers that could potentially be scheduled on the specified link.
        """
        source_se = link_key[0]
        dest_se = link_key[1]

        link_config_max_active = self._get_link_config_max_active(link_key)
        source_out_potential = self._get_storage_outbound_potential(source_se)
        dest_in_potential = self._get_storage_inbound_potential(dest_se)
        max_active = min(
            link_config_max_active, source_out_potential, dest_in_potential
        )
        link_optimizer_limit = self._get_link_optimizer_limit(link_key)
        if link_optimizer_limit is not None:
            max_active = min(max_active, link_optimizer_limit)

        link_nb_active = self._get_link_nb_active(link_key)
        link_potential = LinkPotential(
            max_active=max_active, nb_active=link_nb_active, nb_queued=link_nb_queued
        )
        return link_potential

    def _get_link_key_to_potential(self):
        """
        Returns a map from link to the number of transfers that could potentially be scheduled on
        that link.  The map only contains links that have at least 1 potential transfer.
        """
        link_to_nb_queued = self._get_link_to_nb_queued()
        link_key_to_queues = self._get_link_key_to_queues()
        link_keys = list(link_key_to_queues.keys())

        link_key_to_potential = {}
        for link_key in link_keys:
            link_nb_queued = (
                0 if link_key not in link_to_nb_queued else link_to_nb_queued[link_key]
            )
            link_potential = self._get_link_potential(link_key, link_nb_queued)
            if link_potential.get_potential() > 0:
                link_key_to_potential[link_key] = link_potential
        return link_key_to_potential

    def _get_storage_inbound_max_active(self, storage):
        if storage in self.sched_input["storages_limits"]:
            return self.sched_input["storages_limits"][storage]["inbound_max_active"]
        if "*" in self.sched_input["storages_limits"]:
            return self.sched_input["storages_limits"]["*"]["inbound_max_active"]
        raise Exception(
            "DefaultSchedulerAlgo._get_storage_inbound_max_active(): "
            f"No storage configuration for storage={storage} or storage=*"
        )

    def _get_storage_outbound_max_active(self, storage):
        if storage in self.sched_input["storages_limits"]:
            return self.sched_input["storages_limits"][storage]["outbound_max_active"]
        if "*" in self.sched_input["storages_limits"]:
            return self.sched_input["storages_limits"]["*"]["outbound_max_active"]
        raise Exception(
            "DefaultSchedulerAlgo._get_storage_outbound_max_active(): "
            f"No storage configuration for storage={storage} or storage=*"
        )

    def _get_potential_concurrent_transfers(self):
        """
        Returns the number of potential concurrent-transfers
        """
        nb_active = 0
        for stats in self.sched_input["active_stats"]:
            nb_active += stats["nb_active"]
        return max(0, self.sched_input["max_url_copy_processes"] - nb_active)
