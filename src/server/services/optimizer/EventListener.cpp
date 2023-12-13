
#include "common/Logger.h"
#include "common/Exceptions.h"
#include "EventListener.h"
#include "msg-bus/consumer.h"

namespace fts3
{
    namespace server
    {

        using namespace fts3::common;

        EventListener::EventListener(HeartBeat *beat) : BaseService("EventListener"), beat(beat)
        {
        }

        void EventListener::runEventHandler(fts3::events::TransferCompleted event)
        {
            fts_phase_handler_s ph;
            int ret;
            fts_handler_conf cf;

            ph = cf->handlers;

            while (ph[event->phase_handler].checker)
            {
                ret = ph[event->phase_handler].checker(event, event->phase_handler);

                if (ret == FTS_OK)
                {
                    return;
                }
            }
        }

        void EventListener::phaseChecker(fts3::events::TransferCompleted event, fts_phase_handler_s ph)
        {
            int ret;

            ret = ph->handler(event)
            if (ret == FTS_OPTIMIZE) {
                // handler evaluated conditions
                // and decided to compute gradient
                // now switch to SCHEDULE handler
                event->phase_handler = ph->next;
                return FTS_AGAIN;
            }
            if (ret == FTS_SCHEDULE) {
                // scheduling phase. keep scheduling until 
                // required state is reached
                return FTS_AGAIN;
            }
            if (ret == FTS_DONE) {
                // scheduler handler returned done
                return FTS_OK;
            }
        }

        void EventListener::runService()
        {
            while (true)
            {
                // FTS3_COMMON_LOGGER_NEWLOG(INFO) << "EGG: event listener is running!" << commit;
                // check if this is the lead node
                if (beat->isLeadNode())
                {
                    // FTS3_COMMON_LOGGER_NEWLOG(INFO) << "EGG: it's the lead node!" << commit;
                    listen();
                }
                boost::this_thread::sleep(boost::posix_time::seconds(1));
                // FTS3_COMMON_LOGGER_NEWLOG(INFO) << "EGG: service is running" << commit;
            }

            // if it is, then run the event loop while it is
        }

        void EventListener::listen(void)
        {
            std::vector<std::string> messages;
            Consumer consumer("/var/lib/fts3");
            int returnValue = consumer.runConsumerEvents(messages);
            // look at message queue. if there is a message, run optimizer
            if (returnValue != 0)
            {
                std::ostringstream errorMessage;
                errorMessage << "EGG: runConsumer returned " << returnValue;
                FTS3_COMMON_LOGGER_LOG(ERR, errorMessage.str());
            }

            for (auto iter = messages.begin(); iter != messages.end(); ++iter)
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "EGG: Captured a completion event!" << *iter << commit;
            }
        }
    }
}