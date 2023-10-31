#include "db/generic/QueueId.h"
#include "db/generic/TransferFile.h"

namespace fts3 {
namespace server {

class Scheduler
{
public:
    // TODO: fix after deciding config format
    static const std::string RANDOMIZED_ALGORITHM = "randomized";
    static const std::string DEFICIT_ALGORITHM = "deficit";

    // Function pointer to the scheduler algorithm
    typedef std::map<std::string, std::list<TransferFile>> (*SchedulerFunction)(std::vector<int> slotsPerLink, std::vector<QueueId> queues, int availableUrlCopySlots);

    // Returns function pointer to the scheduler algorithm
    static SchedulerFunction getSchedulingFunction(std::string algorithm);

private:
    // Run randomized scheduling
    static std::map<std::string, std::list<TransferFile>> doRandomizedSchedule(std::vector<int> slotsPerLink, std::vector<QueueId> queues, int availableUrlCopySlots);

    // Run deficit-based priority-queueing scheduling
    static std::map<std::string, std::list<TransferFile>> doDeficitSchedule(std::vector<int> slotsPerLink, std::vector<QueueId> queues, int availableUrlCopySlots);
};

} // end namespace server
} // end namespace fts3
