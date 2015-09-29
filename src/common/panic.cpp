/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "panic.h"
#include <cstring>
#include <execinfo.h>
#include <semaphore.h>
#include <signal.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <boost/thread.hpp>
#include <sys/prctl.h>

/*
 * This file contains the logic to handle signals, logging them and
 * killing the process.
 * Is it this complicated because the signal handler itself should do as little
 * as possible, and must be reentrant. Otherwise, deadlocks may occur.
 * Therefore, we limit the handle to set two flags, and let the logging and killing
 * happen in a separate thread, outside the signal handling logic.
 */

using namespace fts3::common; 

static sem_t semaphore;
static sig_atomic_t raised_signal = 0;

void *panic::stack_backtrace[STACK_BACKTRACE_SIZE] = {0};
int panic::stack_backtrace_size = 0;


static void get_backtrace(int signum)
{
        panic::stack_backtrace_size = backtrace(panic::stack_backtrace, STACK_BACKTRACE_SIZE);

        // print out all the frames to stderr
        fprintf(stderr, "Caught signal: %d\n", signum);
        fprintf(stderr, "Stack trace: \n");
        backtrace_symbols_fd(panic::stack_backtrace, panic::stack_backtrace_size, STDERR_FILENO);
        // and then print out all the frames to stdout
        fprintf(stdout, "Caught signal: %d\n", signum);
        fprintf(stdout, "Stack trace: \n");
        backtrace_symbols_fd(panic::stack_backtrace, panic::stack_backtrace_size, STDOUT_FILENO);
}

// Reset handler and re-raise, so the system handles it
// If ulimit -c is unlimited, a coredump will be generated for
// some of them (as SIGSEGV)
static void delegate_to_default(int signum)
{
    // Change working directory to a writeable directory!
    chdir("/tmp");
    // setxid clears this flag, so need to reset to get the dump
    prctl(PR_SET_DUMPABLE, 1);
    // re-issue and let the system do the work
    signal(signum, SIG_DFL);
    raise(signum);
}

// Minimalistic logic inside a signal!
static void signal_handler(int signum)
{
    if (signum != raised_signal) {
        if (signum == SIGABRT ||
            signum == SIGSEGV ||
            signum ==  SIGILL ||
            signum ==  SIGFPE ||
            signum == SIGBUS ||
            signum ==  SIGTRAP ||
            signum ==  SIGSYS) {
                get_backtrace(signum);
        }
    }
    raised_signal = signum;
    // From man sem_post
    // sem_post() is async-signal-safe: it may be safely called within a signal handler.
    sem_post(&semaphore);

    switch (signum) {
        // Termination signals, do nothing, the installed callback should handle it
        case SIGINT: case SIGUSR1: case SIGTERM:
            break;
        // Let the system handle the rest
        default:
            sleep(30);
            delegate_to_default(signum);
            break;
    }
}

// Thread that logs, waits and kills
static void signal_watchdog(void (*shutdown_callback)(int, void*), void* udata)
{
    int r = 0;
    do
        {
            r = sem_wait(&semaphore);
        }
    while (r < 0);   // Semaphore may return spuriously with errno = EINTR
    shutdown_callback(raised_signal, udata);
}

// Set up the callbacks, and launch the watchdog thread
static void (*_arg_shutdown_callback)(int, void*);
static void *_arg_udata;
static void set_handlers(void)
{
    static const int CATCH_SIGNALS[] =
    {
        SIGABRT, SIGSEGV, SIGILL, SIGFPE,
        SIGBUS, SIGTRAP, SIGSYS,
        SIGQUIT, SIGINT, SIGUSR1, SIGTERM
    };
    static const size_t N_CATCH_SIGNALS = sizeof(CATCH_SIGNALS) / sizeof(int);
    static struct sigaction actions[N_CATCH_SIGNALS];

    sem_init(&semaphore, 0, 0);

    static sigset_t proc_mask;
    sigemptyset(&proc_mask);

    memset(actions, 0, sizeof(actions));
    for (size_t i = 0; i < N_CATCH_SIGNALS; ++i)
        {
            actions[i].sa_handler = &signal_handler;
            sigemptyset(&actions[i].sa_mask);
            actions[i].sa_flags = SA_RESTART;
            sigaction(CATCH_SIGNALS[i], &actions[i], NULL);
            sigaddset(&proc_mask, CATCH_SIGNALS[i]);
        }

    // Unblock signals (daemon may have blocked some of them)
    sigprocmask(SIG_UNBLOCK, &proc_mask, NULL);

    boost::thread watchdog(signal_watchdog, _arg_shutdown_callback, _arg_udata);
}

// Wrap set_handlers, so it is called only once
void panic::setup_signal_handlers(void (*shutdown_callback)(int, void*), void* udata)
{
    // First thing, wait for a signal to be caught
    static boost::once_flag set_handlers_flag = BOOST_ONCE_INIT;
    _arg_shutdown_callback = shutdown_callback;
    _arg_udata = udata;
    boost::call_once(&set_handlers, set_handlers_flag);
}


std::string panic::stack_dump(void* array[], int stack_size)
{
    std::string stackTrace;

    char ** symbols = backtrace_symbols(array, stack_size);
    for (register int i = 0; i < stack_size; ++i)
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

    return stackTrace;
}
