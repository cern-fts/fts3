from scheduler_algo import SchedulerAlgo, SchedulerDecision


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
            raise Exception(f"remove_value(): {e}")

        # Wrap next_idx around to 0 if has fallen off the buffer
        if self._next_idx == len(self._buf):
            self._next_idx = 0


class DefaultSchedulerAlgo(SchedulerAlgo):
    def schedule(self) -> SchedulerDecision:
        potential_concurrent_transfers = self._get_potential_concurrent_transfers()

        # Do nothing if no work to be done or concurrent transfer limit reached
        if not self.sched_input["queues"] or not potential_concurrent_transfers:
            return None

        link_to_nb_queued = self._get_link_to_nb_queued()
        link_to_queues = self._get_link_to_queues()
        all_link_keys = list(link_to_queues.keys())
        link_to_potential = self._get_link_to_potential(
            all_link_keys, link_to_nb_queued
        )

        # Do nothing if there are no link with the potential of being scheduled
        if not link_to_potential:
            return None

        # Create a circular buffer of the sorted IDs of the links with the potential to a schedule
        # transfer
        potential_link_keys = list(link_to_potential.keys())
        potential_link_keys.sort()
        potential_link_key_cbuf = CircularBuffer()
        for potential_link_key in potential_link_keys:
            potential_link_key_cbuf.append(potential_link_key)

        # Create a circular buffer of queue_ids for each link
        potential_link_to_queue_id_cbuf = {}
        for potential_link_key in potential_link_keys:
            queues = link_to_queues[potential_link_key]
            queue_ids = list(queues.keys())
            queue_ids.sort()
            queue_id_cbuf = CircularBuffer()
            for queue_id in queue_ids:
                queue_id_cbuf.append(queue_id)
            potential_link_to_queue_id_cbuf[potential_link_key] = queue_id_cbuf

        if self.sched_input["opaque_data"]:
            potential_link_key_cbuf.skip_until_after(
                self.sched_input["opaque_data"]["id_of_last_scheduled_link"]
            )

        # Round robin free work-capacity across submission queues taking into account any
        # constraints
        scheduler_decision = SchedulerDecision()
        for i in range(potential_concurrent_transfers):
            # Identify link that could do work
            link_key = potential_link_key_cbuf.get_next()

            # Get the next eligble queue on this link
            queue_id_cbuf = potential_link_to_queue_id_cbuf[link_key]
            queue_id = queue_id_cbuf.get_next()

            # Schedule a transfer for this queue
            scheduler_decision.inc_transfers_for_queue(queue_id, 1)
            if not scheduler_decision.get_opaque_data():
                scheduler_decision.set_opaque_data({})
            scheduler_decision.get_opaque_data()["id_of_last_scheduled_link"] = link_key

            # Update links to reflect remaining work to be done
            link_to_potential[link_key] = link_to_potential[link_key] - 1
            if link_to_potential[link_key] == 0:
                potential_link_key_cbuf.remove_value(link_key)

            # Stop scheduling if there is no more work to be done
            if not potential_link_key_cbuf:
                break

        return scheduler_decision

    def _get_link_to_queues(self):
        link_to_queues = {}
        for queue_id, queue in self.sched_input["queues"].items():
            link_key = (queue["source_se"], queue["dest_se"])
            if link_key not in link_to_queues.keys():
                link_to_queues[link_key] = {}
            link_to_queues[link_key][queue_id] = queue
        return link_to_queues

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

    def _get_link_nb_active(self, link_key):
        if link_key in self.sched_input["active_links"]:
            return self.sched_input["active_links"][link_key]["nb_active"]
        else:
            return 0

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
        source_out_max_active = self._get_storage_outbound_max_active(source_se)
        dest_in_max_active = self._get_storage_inbound_max_active(source_se)

        max_active = min(link_max_active, source_out_max_active, dest_in_max_active)

        link_nb_active = self._get_link_nb_active(link_key)
        link_potential = min(link_nb_queued, max(0, max_active - link_nb_active))
        return link_potential

    def _get_link_to_potential(self, link_keys, link_to_nb_queued):
        """
        Returns a map from link to the number of transfers that could potentionally be scheduled on
        that link.  The map only contains links that have at least 1 potential transfer.
        """
        link_to_potential = {}
        for link_key in link_keys:
            link_nb_queued = (
                0 if link_key not in link_to_nb_queued else link_to_nb_queued[link_key]
            )
            link_potential = self._get_link_potential(link_key, link_nb_queued)
            if link_potential > 0:
                link_to_potential[link_key] = link_potential
        return link_to_potential

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
        for active_link_key, active_link in self.sched_input["active_links"].items():
            nb_active += active_link["nb_active"]
        return max(0, self.sched_input["max_url_copy_processes"] - nb_active)
