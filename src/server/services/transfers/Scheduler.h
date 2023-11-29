#include "db/generic/QueueId.h"
#include "db/generic/TransferFile.h"

#include "VoShares.h"

namespace fts3 {
namespace server {

class Scheduler
{
public:
    // TODO: fix after deciding config format
    static const std::string RANDOMIZED_ALGORITHM;
    static const std::string DEFICIT_ALGORITHM;

    // Define LinkVoActivityKey, which is (src, dest, vo, activity)
    using LinkVoActivityKey = std::tuple<std::string, std::string, std::string, std::string>;

    // Function pointer to the scheduler algorithm
    using SchedulerFunction = std::map<std::string, std::list<TransferFile>> (*)(std::map<Pair, int>&, std::vector<QueueId>&, int);

    // Returns function pointer to the scheduler algorithm
    static SchedulerFunction getSchedulingFunction(std::string algorithm);

private:
    // The scheduling functions below should execute the corresponding
    // scheduling algorithm, and they should return a mapping from
    // VOs to the list of TransferFiles to be scheduled by TransferService.
    //
    // Note that the list of TransferFiles returned here has not yet accounted
    // for the number of slots available for src/dest nodes. This will happen
    // in TransferService.

    /**
     * Run scheduling using weighted randomization.
     * @param slotsPerLink number of slots assigned to each link, as determined by allocator
     * @param queues All current pending transfers
     * @param availableUrlCopySlots Max number of slots available in the system
     * @return Mapping from each VO to the list of transfers to be scheduled.
     */
    static std::map<std::string, std::list<TransferFile>> doRandomizedSchedule(std::map<Pair, int> &slotsPerLink, std::vector<QueueId> &queues, int availableUrlCopySlots);

    // Run deficit-based priority-queueing scheduling
    static std::map<std::string, std::list<TransferFile>> doDeficitSchedule(std::map<Pair, int> &slotsPerLink, std::vector<QueueId> &queues, int availableUrlCopySlots);
};

const std::string Scheduler::RANDOMIZED_ALGORITHM = "RANDOMIZED";
const std::string Scheduler::DEFICIT_ALGORITHM = "DEFICIT";

} // end namespace server
} // end namespace fts3
