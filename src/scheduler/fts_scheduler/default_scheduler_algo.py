from scheduler_algo import SchedulerAlgo, SchedulerDecision


class CirculerBuffer:
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

        link_id_to_nb_queued = self._get_link_id_to_nb_queued()
        link_id_to_queues = self._get_link_id_to_queues()
        all_link_ids = list(link_id_to_queues.keys())
        link_id_to_potential = self._get_link_id_to_potential(
            all_link_ids, link_id_to_nb_queued
        )

        # Do nothing if there are no link with the potential of being scheduled
        if not link_id_to_potential:
            return None

        # Create a circular buffer of the sorted IDs of the links with the potential to a schedule
        # transfer
        potential_link_ids = list(link_id_to_potential.keys())
        potential_link_ids.sort()
        potential_link_id_cbuf = CirculerBuffer()
        for potential_link_id in potential_link_ids:
            potential_link_id_cbuf.append(potential_link_id)

        # Create a circular buffer of queue_ids for each link
        potential_link_id_to_queue_id_cbuf = {}
        for potential_link_id in potential_link_ids:
            queues = link_id_to_queues[potential_link_id]
            queue_ids = list(queues.keys())
            queue_ids.sort()
            queue_id_cbuf = CirculerBuffer()
            for queue_id in queue_ids:
                queue_id_cbuf.append(queue_id)
            potential_link_id_to_queue_id_cbuf[potential_link_id] = queue_id_cbuf

        if self.sched_input["opaque_data"]:
            potential_link_id_cbuf.skip_until_after(
                self.sched_input["opaque_data"]["id_of_last_scheduled_link"]
            )

        # Round robin free work-capacity across submission queues taking into account any
        # constraints
        scheduler_decision = SchedulerDecision()
        for i in range(potential_concurrent_transfers):
            # Identify link that could do work
            link_id = potential_link_id_cbuf.get_next()

            # Get the next eligble queue on this link
            queue_id_cbuf = potential_link_id_to_queue_id_cbuf[link_id]
            queue_id = queue_id_cbuf.get_next()

            # Schedule a transfer for this queue
            scheduler_decision.inc_transfers_for_queue(queue_id, 1)
            if not scheduler_decision.get_opaque_data():
                scheduler_decision.set_opaque_data({})
            scheduler_decision.get_opaque_data()["id_of_last_scheduled_link"] = link_id

            # Update links to reflect remaining work to be done
            link_id_to_potential[link_id] = link_id_to_potential[link_id] - 1
            if link_id_to_potential[link_id] == 0:
                potential_link_id_cbuf.remove_value(link_id)

            # Stop scheduling if there is no more work to be done
            if not potential_link_id_cbuf:
                break

        return scheduler_decision

    def _get_link_id_to_queues(self):
        link_id_to_queues = {}
        for queue_id, queue in self.sched_input["queues"].items():
            link_id = (queue["source_se"], queue["dest_se"])
            if link_id not in link_id_to_queues.keys():
                link_id_to_queues[link_id] = {}
            link_id_to_queues[link_id][queue_id] = queue
        return link_id_to_queues

    def _get_link_max_active(self, link_id):
        max_active = 0

        if link_id in self.sched_input["link_limits"].keys():
            max_active = self.sched_input["link_limits"][link_id]["max_active"]
        elif ("*", "*") in self.sched_input["link_limits"].keys():
            max_active = self.sched_input["link_limits"][("*", "*")]["max_active"]

        if link_id in self.sched_input["optimizer_limits"].keys():
            max_active = min(
                max_active, self.sched_input["optimizer_limits"][link_id]["active"]
            )

        return max_active

        raise Exception(
            "DefaultSchedulerAlgo._get_link_max_active(): No link configuration found for "
            "(source_se={source_se},dest_se={dest_se}) or (source_se=*, dest_se=*)"
        )

    def _get_link_nb_active(self, link_id):
        if link_id in self.sched_input["active_links"]:
            return self.sched_input["active_links"][link_id]["nb_active"]
        else:
            return 0

    def _get_link_id_to_nb_queued(self):
        link_id_to_nb_queued = {}
        for queue_id, queue in self.sched_input["queues"].items():
            link_id = (queue["source_se"], queue["dest_se"])
            if link_id not in link_id_to_nb_queued.keys():
                link_id_to_nb_queued[link_id] = queue["nb_files"]
            else:
                link_id_to_nb_queued[link_id] += queue["nb_files"]
        return link_id_to_nb_queued

    def _get_link_potential(self, link_id, link_nb_queued):
        """
        Returns the number of transfers that could potentionally be scheduled on the specified link.
        This method takes in to account the bot link and storage endpoint constraints.
        """
        source_se = link_id[0]
        dest_se = link_id[1]

        link_max_active = self._get_link_max_active(link_id)
        source_out_max_active = self._get_storage_outbound_max_active(source_se)
        dest_in_max_active = self._get_storage_inbound_max_active(source_se)

        max_active = min(link_max_active, source_out_max_active, dest_in_max_active)

        link_nb_active = self._get_link_nb_active(link_id)
        link_potential = min(link_nb_queued, max(0, max_active - link_nb_active))
        return link_potential

    def _get_link_id_to_potential(self, link_ids, link_id_to_nb_queued):
        """
        Returns a map from link ID to the number of transfers that could potentionally be scheduled
        on the corresponding link.  The map only contains links that have at least 1 potential
        transfer.
        """
        link_id_to_potential = {}
        for link_id in link_ids:
            link_nb_queued = (
                0
                if link_id not in link_id_to_nb_queued
                else link_id_to_nb_queued[link_id]
            )
            link_potential = self._get_link_potential(link_id, link_nb_queued)
            if link_potential > 0:
                link_id_to_potential[link_id] = link_potential
        return link_id_to_potential

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
        for active_link_id, active_link in self.sched_input["active_links"].items():
            nb_active += active_link["nb_active"]
        return max(0, self.sched_input["max_url_copy_processes"] - nb_active)
