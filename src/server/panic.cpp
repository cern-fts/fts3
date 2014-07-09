/* Copyright @ CERN, 2014.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "panic.h"
#include "server.h"
#include "common/logger.h"
#include "db/generic/SingleDbInstance.h"
#include <cstring>
#include <execinfo.h>
#include <semaphore.h>
#include <signal.h>

/*
 * This file contains the logic to handle signals, logging them and
 * killing the process.
 * Is it this complicated because the signal handler itself should do as little
 * as possible, and must be reentrant. Otherwise, deadlocks may occur.
 * Therefore, we limit the handle to set two flags, and let the logging and killing
 * happen in a separate thread, outside the signal handling logic.
 */

using namespace FTS3_SERVER_NAMESPACE;
using namespace FTS3_COMMON_NAMESPACE;

static sem_t semaphore;
static int raised_signal = 0;

// Minimalistic logic inside a signal!
static void signal_handler(int signal)
{
    extern bool stopThreads;

    stopThreads = true;
    raised_signal = signal;

    // From man sem_post
    // sem_post() is async-signal-safe: it may be safely called within a signal handler.
    sem_post(&semaphore);
}

// Log the stack
static void log_stack(void)
{
    std::string stackTrace;

    const int stack_size = 25;
    void * array[stack_size]= {0};
    int nSize = backtrace(array, stack_size);
    char ** symbols = backtrace_symbols(array, nSize);
    for (register int i = 0; i < nSize; ++i)
        {
            if(symbols && symbols[i])
                {
                    stackTrace+=std::string(symbols[i]) + '\n';
                }
        }
    if(symbols)
        {
            free(symbols);
        }

    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Stack trace: \n" << stackTrace << commit;
}

// Thread that logs, waits and kills
static void signal_watchdog(void)
{
    sem_wait(&semaphore);

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Caught signal " << raised_signal
            << " (" << strsignal(raised_signal) << ")" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Future signals will be ignored!" << commit;

    // Some require traceback
    int exit_status = 0;

    switch (raised_signal)
        {
            case SIGABRT: case SIGSEGV: case SIGTERM:
            case SIGILL: case SIGFPE: case SIGBUS:
            case SIGTRAP: case SIGSYS:
                exit_status = -raised_signal;
                log_stack();
                break;
            default:
                break;
        }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopping" << commit;
    sleep(15);
    try
        {
            theServer().stop();
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS db connections closing" << commit;
            db::DBSingleton::tearDown();
            sleep(10);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS db connections closed" << commit;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception when forcing the database teardown" << commit;
        }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopped" << commit;
    _exit(exit_status);
}

// Set up the callbacks, and launch the watchdog thread
static void set_handlers(void)
{
    static const int CATCH_SIGNALS[] = {
        SIGABRT, SIGSEGV, SIGTERM,
        SIGILL, SIGFPE, SIGBUS,
        SIGTRAP, SIGSYS,
        SIGINT, SIGTERM, SIGUSR1
    };
    static const size_t N_CATCH_SIGNALS = sizeof(CATCH_SIGNALS) / sizeof(int);
    static struct sigaction actions[N_CATCH_SIGNALS];

    sem_init(&semaphore, 0, 0);

    memset(actions, 0, sizeof(actions));
    for (size_t i = 0; i < N_CATCH_SIGNALS; ++i) {
        actions[i].sa_handler = &signal_handler;
        sigemptyset(&actions[i].sa_mask);
        actions[i].sa_flags = SA_RESTART;
        sigaction(CATCH_SIGNALS[i], &actions[i], NULL);
    }

    boost::thread watchdog(signal_watchdog);

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Signal handlers installed" << commit;
}

// Wrap set_handlers, so it is called only once
void Panic::setup_signal_handlers()
{
    // First thing, wait for a signal to be caught
    static boost::once_flag set_handlers_flag = BOOST_ONCE_INIT;
    boost::call_once(&set_handlers, set_handlers_flag);
}
