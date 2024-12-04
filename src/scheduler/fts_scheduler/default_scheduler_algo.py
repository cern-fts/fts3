from scheduler_algo import SchedulerAlgo, SchedulerDecision
import copy


class CircularBuffer:
    def __init__(self):
        self._buf = []
        self._next_idx = 0

    def append(self, value):
        self._buf.append(value)

    def get_next(self):
        if not self._buf:
            raise Exception("get_next(): Empty buffer")
        next_value = self._buf[self._next_idx]
        self._next_idx = (self._next_idx + 1) % len(self._buf)
        return next_value

    def get_buf_deep_copy(self):
        return copy.deepcopy(self._buf)

    def __len__(self):
        return len(self._buf)

    def __bool__(self):
        return bool(self._buf)

    def __str__(self):
        return (
            "{"
            + f"buf={str(self._buf)},"
            + f"next_idx={self._next_idx},"
            + f"next={self._buf[self._next_idx] if self._buf else None}"
            "}"
        )

    def __contains__(self, item):
        return item in self._buf

    def skip_until_after(self, val_to_skip_over):
        """
        Skip through this circular-buffer until after the specified value
        """
        if not self._buf:
            raise Exception("skip_until_after(): Circular-buffer is empty")

        for idx in range(len(self._buf)):
            if self._buf[idx] > val_to_skip_over:
                self._next_idx = idx
                return
        self._next_idx = 0

    def remove_value(self, value):
        try:
            self._buf.remove(value)
        except Exception as e:
            raise Exception(f"remove_value(): value={value}: {e}")

        # Wrap next_idx around to 0 if has fallen off the buffer
        if self._next_idx == len(self._buf):
            self._next_idx = 0


class DefaultSchedulerAlgo(SchedulerAlgo):
    def schedule(self) -> SchedulerDecision:
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

        # Create a circular-buffer of the keys of links with the potential to schedule a transfer
        potential_link_key_cbuf = CircularBuffer()
        for link_key in potential_link_keys:
            potential_link_key_cbuf.append(link_key)

        # Create link key -> VO circular-buffer
        potential_link_to_vo_cbuf = {}
        for link_key in potential_link_keys:
            vo_cbuf = CircularBuffer()
            for vo in sorted(link_to_vo_to_activity_to_queue[link_key].keys()):
                vo_cbuf.append(vo)
            potential_link_to_vo_cbuf[link_key] = vo_cbuf

        # Create link-key -> VO -> activity circular-buffer
        potential_link_to_vo_to_activity_cbuf = {}
        for link_key in potential_link_keys:
            potential_link_to_vo_to_activity_cbuf[link_key] = {}
            vos = link_to_vo_to_activity_to_queue[link_key].keys()
            for vo in vos:
                activity_cbuf = CircularBuffer()
                for activity in sorted(
                    link_to_vo_to_activity_to_queue[link_key][vo].keys()
                ):
                    activity_cbuf.append(activity)
                potential_link_to_vo_to_activity_cbuf[link_key][vo] = activity_cbuf

        # Create link-key -> VO -> activity -> queue-id
        potential_link_to_vo_to_activity_to_queue_id = {}
        for link_key in potential_link_keys:
            potential_link_to_vo_to_activity_to_queue_id[link_key] = {}
            for vo, activity_to_queue in link_to_vo_to_activity_to_queue[
                link_key
            ].items():
                potential_link_to_vo_to_activity_to_queue_id[link_key][vo] = {}
                for activity, queue in activity_to_queue.items():
                    queue_id = queue["queue_id"]
                    potential_link_to_vo_to_activity_to_queue_id[link_key][vo][
                        activity
                    ] = queue_id

        # Fast-forward circular buffers based on previous scheduling run
        if self.sched_input["opaque_data"]:
            sched_opaque_data = self.sched_input["opaque_data"]
            if "id_of_last_scheduled_link" in sched_opaque_data:
                potential_link_key_cbuf.skip_until_after(
                    sched_opaque_data["id_of_last_scheduled_link"]
                )
            if "link_to_last_scheduled_vo" in sched_opaque_data:
                for link_key, vo in sched_opaque_data[
                    "link_to_last_scheduled_vo"
                ].items():
                    if link_key in potential_link_to_vo_cbuf:
                        potential_link_to_vo_cbuf[link_key].skip_until_after(vo)
            if "link_to_vo_to_last_scheduled_activity" in sched_opaque_data:
                for link_key, vo_to_activity in sched_opaque_data[
                    "link_to_vo_to_last_scheduled_activity"
                ].items():
                    if link_key in potential_link_to_vo_to_activity_cbuf:
                        for vo, activity in vo_to_activity.items():
                            if vo in potential_link_to_vo_to_activity_cbuf[link_key]:
                                potential_link_to_vo_to_activity_cbuf[link_key][
                                    vo
                                ].skip_until_after(activity)

        # To be iteratively modified in order to know when to stop considering a VO
        link_key_to_vo_to_nb_queued = self._get_link_to_vo_to_nb_queued()

        # To be iteratively modified in order to know when to stop considering an activity
        link_key_to_vo_to_activity_to_nb_queued = (
            self._get_link_key_to_vo_to_activity_to_nb_queued()
        )

        # Round robin free work-capacity across submission queues taking into account any
        # constraints
        scheduler_decision = SchedulerDecision()
        for i in range(potential_concurrent_transfers):
            # Identify the link and storages that could do the work
            link_key = potential_link_key_cbuf.get_next()
            source_se = link_key[0]
            dest_se = link_key[1]

            # Get the circular buffer of VOs on the link
            vo_cbuf = potential_link_to_vo_cbuf[link_key]

            # Get the next VO of the link
            vo = vo_cbuf.get_next()

            # Get the circular buffer of activities of the VO on the link
            activity_cbuf = potential_link_to_vo_to_activity_cbuf[link_key][vo]

            # Get the next activity of the VO on the link
            activity = activity_cbuf.get_next()

            # Get the ID of the next eligble queue
            queue_id = potential_link_to_vo_to_activity_to_queue_id[link_key][vo][
                activity
            ]

            # Schedule a transfer for this queue
            scheduler_decision.inc_transfers_for_queue(queue_id, 1)

            # Update the scheduling opaque-data
            if not scheduler_decision.get_opaque_data():
                scheduler_decision.set_opaque_data({})
                scheduler_decision.get_opaque_data()["link_to_last_scheduled_vo"] = {}
                scheduler_decision.get_opaque_data()[
                    "link_to_vo_to_last_scheduled_activity"
                ] = {}
            opaque_data = scheduler_decision.get_opaque_data()
            opaque_data["id_of_last_scheduled_link"] = link_key
            opaque_data["link_to_last_scheduled_vo"][link_key] = vo
            if link_key not in opaque_data["link_to_vo_to_last_scheduled_activity"]:
                opaque_data["link_to_vo_to_last_scheduled_activity"][link_key] = {}
            opaque_data["link_to_vo_to_last_scheduled_activity"][link_key][
                vo
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
            link_key_to_potential[link_key] = link_key_to_potential[link_key] - 1

            # Apply storage potential updates to link potentials
            for _, link_potential in link_key_to_potential.items():
                link_potential = min(
                    link_potential,
                    storage_to_outbound_potential[source_se],
                    storage_to_inbound_potential[dest_se],
                )

            # Remove saturated links from circular buffer
            staturated_links = [
                link_key
                for link_key, potential in link_key_to_potential.items()
                if potential == 0
            ]
            for staturated_link_key in staturated_links:
                if staturated_link_key in potential_link_key_cbuf:
                    potential_link_key_cbuf.remove_value(staturated_link_key)

            # Remove VO from VO circular-buffer if necessary
            link_key_to_vo_to_nb_queued[link_key][vo] -= 1
            if link_key_to_vo_to_nb_queued[link_key][vo] < 0:
                raise Exception(
                    f"Link to VO to nb_queued went negative: source_se={source_se} dest_se={dest_se} vo={vo}"
                )
            if link_key_to_vo_to_nb_queued[link_key][vo] == 0:
                vo_cbuf.remove_value(vo)

            # Remove activity from activty circular-buffer if necessary
            link_key_to_vo_to_activity_to_nb_queued[link_key][vo][activity] -= 1
            if link_key_to_vo_to_activity_to_nb_queued[link_key][vo][activity] < 0:
                raise Exception(
                    f"Link to VO to activity to nb_queued went negative: source_se={source_se} dest_se={dest_se} vo={vo} activity={activity}"
                )
            if link_key_to_vo_to_activity_to_nb_queued[link_key][vo][activity] == 0:
                activity_cbuf.remove_value(activity)

            # Stop scheduling if there is no more work to be done
            if not potential_link_key_cbuf:
                break

        return scheduler_decision

    def _get_link_to_vo_to_nb_queued(self):
        result = {}
        for queue_id, queue in self.sched_input["queues"].items():
            vo = queue["vo_name"]
            source_se = queue["source_se"]
            dest_se = queue["dest_se"]
            link_key = (source_se, dest_se)
            nb_files = queue["nb_files"]

            if link_key not in result:
                result[link_key] = {}
            result[link_key][vo] = (
                nb_files
                if vo not in result[link_key]
                else result[link_key][vo] + nb_files
            )
        return result

    def _get_vo_activities_of_queues(self, queue_ids):
        vo_activities = {}  # Activities are grouped by VO
        for queue_id in queue_ids:
            queue = self.sched_input["queues"][queue_id]
            vo = queue["vo_name"]
            activity = queue["activity"]
            if vo not in vo_activities:
                vo_activities[vo] = []
            vo_activities[vo].append(activity)
        return vo_activities

    def _get_vo_total_activity_weights(self, vo_activities):
        vo_total_activity_weights = {}
        for vo in vo_activities.keys():
            activities = vo_activities[vo]
            activity_share = self.sched_input["vo_activity_shares"][vo]
            total = 0
            for activity in activities:
                total += activity_share[activity]
            vo_total_activity_weights[vo] = total
        return vo_total_activity_weights

    def _get_link_key_to_queues(self):
        link_key_to_queues = {}
        for queue_id, queue in self.sched_input["queues"].items():
            link_key = (queue["source_se"], queue["dest_se"])
            if link_key not in link_key_to_queues.keys():
                link_key_to_queues[link_key] = {}
            link_key_to_queues[link_key][queue_id] = queue
        return link_key_to_queues

    def _get_link_to_vo_to_activity_to_queue(self):
        result = {}
        for queue_id, queue in self.sched_input["queues"].items():
            link_key = (queue["source_se"], queue["dest_se"])
            vo = queue["vo_name"]
            activity = queue["activity"]

            if link_key not in result:
                result[link_key] = {}
            if vo not in result[link_key]:
                result[link_key][vo] = {}
            if activity not in result:
                result[activity] = {}
            result[link_key][vo][activity] = queue
        return result

    def _get_link_key_to_vo_to_activity_to_nb_queued(self):
        result = {}
        for queue_id, queue in self.sched_input["queues"].items():
            link_key = (queue["source_se"], queue["dest_se"])
            vo = queue["vo_name"]
            activity = queue["activity"]
            nb_files = queue["nb_files"]

            if link_key not in result:
                result[link_key] = {}
            if vo not in result[link_key]:
                result[link_key][vo] = {}
            if activity not in result:
                result[activity] = {}
            result[link_key][vo][activity] = nb_files
        return result

    def _get_link_max_active(self, link_key):
        """
        Returns the maximum number of concurrent transfers allowed on the specified link.  This
        decision takes into account:
            1. The link limit
            2. The source storage-endpoint limit
            3. The destination storage-endpoint limit
            4. The optimizer limit
        """
        max_active = 0

        if link_key in self.sched_input["link_limits"].keys():
            max_active = self.sched_input["link_limits"][link_key]["max_active"]
        elif ("*", "*") in self.sched_input["link_limits"].keys():
            max_active = self.sched_input["link_limits"][("*", "*")]["max_active"]
        else:
            raise Exception(
                "DefaultSchedulerAlgo._get_link_max_active(): No link configuration found for "
                "(source_se={source_se},dest_se={dest_se}) or (source_se=*, dest_se=*)"
            )

        if link_key in self.sched_input["optimizer_limits"].keys():
            max_active = min(
                max_active, self.sched_input["optimizer_limits"][link_key]["active"]
            )

        return max_active

    def _get_storage_to_outbound_active(self):
        result = {}
        for link_key, link in self.sched_input["active_link_stats"].items():
            storage = link["source_se"]
            nb_active = link["nb_active"]
            result[storage] = (
                0 if storage not in result else result[storage] + nb_active
            )
        return result

    def _get_storage_to_inbound_active(self):
        result = {}
        for link_key, link in self.sched_input["active_link_stats"].items():
            storage = link["dest_se"]
            nb_active = link["nb_active"]
            result[storage] = (
                0 if storage not in result else result[storage] + nb_active
            )
        return result

    def _get_storages_with_outbound_queues(self):
        result = set()
        for queue_id, queue in self.sched_input["queues"].items():
            result.add(queue["source_se"])
        return result

    def _get_storages_with_inbound_queues(self):
        result = set()
        for queue_id, queue in self.sched_input["queues"].items():
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
        for link_key, link in self.sched_input["active_link_stats"].items():
            nb_active += link["nb_active"] if storage == link["source_se"] else 0
        return nb_active

    def _get_storage_inbound_active(self, storage):
        nb_active = 0
        for link_key, link in self.sched_input["active_link_stats"].items():
            nb_active += link["nb_active"] if storage == link["dest_se"] else 0
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
        nb_active = 0
        for active_link_key, link in self.sched_input["active_link_stats"]:
            nb_active += link["nb_files"] if link_key == active_link_key else 0
        return nb_active

    def _get_link_to_nb_queued(self):
        link_to_nb_queued = {}
        for queue_id, queue in self.sched_input["queues"].items():
            link_key = (queue["source_se"], queue["dest_se"])
            if link_key not in link_to_nb_queued.keys():
                link_to_nb_queued[link_key] = queue["nb_files"]
            else:
                link_to_nb_queued[link_key] += queue["nb_files"]
        return link_to_nb_queued

    def _get_link_nb_active_per_vo(self, link_key):
        nb_active_per_vo = {}
        for queue_id, queue in self.sched_input["queues"].items():
            queue_link_key = (queue["source_se"], queue["dest_se"])
            if queue_link_key == link_key:
                vo = queue["vo"]
                nb_files = queue["nb_files"]

                if vo not in nb_active_per_vo.keys():
                    nb_active_per_vo[vo] = 0
                nb_active_per_vo[vo] += nb_files
        return nb_active_per_vo

    def _get_link_nb_active_per_vo_activity(self, link_key):
        nb_active_per_vo_activity = {}
        for queue_id, queue in self.sched_input["queues"].items():
            queue_link_key = (queue["source_se"], queue["dest_se"])
            if queue_link_key == link_key:
                vo = queue["vo"]
                activity = queue["activity"]
                nb_files = queue["nb_files"]

                if vo not in nb_active_per_vo_activity.keys():
                    nb_active_per_vo_activity[vo] = {}
                if activity not in nb_active_per_vo_activity[vo].keys():
                    nb_active_per_vo_activity[vo][activity] = 0
                nb_active_per_vo_activity[vo][activity] += nb_files
        return nb_active_per_vo_activity

    def _get_link_potential(self, link_key, link_nb_queued):
        """
        Returns the number of transfers that could potentionally be scheduled on the specified link.
        """
        source_se = link_key[0]
        dest_se = link_key[1]

        link_max_active = self._get_link_max_active(link_key)
        source_out_potential = self._get_storage_outbound_potential(source_se)
        dest_in_potential = self._get_storage_inbound_potential(dest_se)

        max_active = min(link_max_active, source_out_potential, dest_in_potential)

        link_nb_active = self._get_link_nb_active(link_key)
        link_potential = min(link_nb_queued, max(0, max_active - link_nb_active))
        return link_potential

    def _get_link_key_to_potential(self):
        """
        Returns a map from link to the number of transfers that could potentionally be scheduled on
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
            if link_potential > 0:
                link_key_to_potential[link_key] = link_potential
        return link_key_to_potential

    def _get_storage_inbound_max_active(self, storage):
        if storage in self.sched_input["storages_limits"]:
            return self.sched_input["storages_limits"][storage]["inbound_max_active"]
        elif "*" in self.sched_input["storages_limits"]:
            return self.sched_input["storages_limits"]["*"]["inbound_max_active"]
        else:
            raise Exception(
                "DefaultSchedulerAlgo._get_storage_inbound_max_active(): "
                f"No storage configuration for storage={storage} or storage=*"
            )

    def _get_storage_outbound_max_active(self, storage):
        if storage in self.sched_input["storages_limits"]:
            return self.sched_input["storages_limits"][storage]["outbound_max_active"]
        elif "*" in self.sched_input["storages_limits"]:
            return self.sched_input["storages_limits"]["*"]["outbound_max_active"]
        else:
            raise Exception(
                "DefaultSchedulerAlgo._get_storage_outbound_max_active(): "
                f"No storage configuration for storage={storage} or storage=*"
            )

    def _get_potential_concurrent_transfers(self):
        """
        Returns the number of potential concurrent-transfers
        """
        nb_active = 0
        for link_key, link in self.sched_input["active_link_stats"].items():
            nb_active += link["nb_active"]
        return max(0, self.sched_input["max_url_copy_processes"] - nb_active)
