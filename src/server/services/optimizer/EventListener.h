
#pragma once
#ifndef EVENTLISTENER_H_
#define EVENTLISTENER_H_

#include "../BaseService.h"
#include "../heartbeat/HeartBeat.h"

typedef enum
{
    // example events
    FTS_OPTIMIZE = 0,
    FTS_SCHEDULE,
    FTS_OK,
} fts_event_handler_phases;

typedef void (*fts_checker_p)();
typedef void (*fts_handler_p)();
typedef void (*fts_handler_conf)();

struct fts_phase_handler_s
{
    fts_checker_p checker;
    fts_handler_p handler;
    int next;
};

namespace fts3
{
    namespace server
    {

        class EventListener : public BaseService
        {
        protected:
            HeartBeat *beat;

        public:
            EventListener(HeartBeat *beat);
            virtual void runService();

        private:
            virtual void listen();
            virtual void runEventHandler(fts3::events::TransferCompleted);
        };

    } // end namespace server
} // end namespace fts3

#endif // EVENTLISTENER_H_
