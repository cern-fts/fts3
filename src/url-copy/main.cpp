/*
 * Copyright (c) CERN 2016
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

#include "common/Logger.h"
#include "common/panic.h"

#include "LogHelper.h"
#include "UrlCopyOpts.h"
#include "UrlCopyProcess.h"
#include "LegacyReporter.h"

#include <cstdlib>

using fts3::common::commit;
namespace panic = fts3::common::panic;


/// Signal handler
static void signalCallback(int signum, void *udata)
{
    UrlCopyProcess *urlCopyProcess = (UrlCopyProcess*)(udata);
    std::stringstream errMsg;
    std::string stackTrace;

    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Received signal " << signum << commit;
    switch (signum) {
        // Fatal errors. Do not try to recover nicely, just send out termination messages.
        // Remember, the default signal handler will generate a coredump and exit.
        case SIGABRT: case SIGSEGV: case SIGILL: case SIGFPE: case SIGBUS:
        case SIGTRAP: case SIGSYS:
            stackTrace = panic::stack_dump(panic::stack_backtrace, panic::stack_backtrace_size);
            errMsg << "Transfer process died with: " << stackTrace;

            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Stacktrace: " << stackTrace << commit;

            urlCopyProcess->panic(errMsg.str());
            break;
        // Termination signal. The process can continue once the cancellation has been triggered.
        case SIGINT: case SIGTERM:
            urlCopyProcess->cancel();
            break;
    }
}

/// Remove some environment variables that may interfere
void clearEnvironment()
{
    unsetenv("X509_USER_CERT");
    unsetenv("X509_USER_KEY");
    unsetenv("X509_USER_PROXY");
}


int main(int argc, char *argv[])
{
    if (getuid() == 0 || geteuid() == 0) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Running as root! This is not recommended." << commit;
    }

    clearEnvironment();

    // Parse options and setup log levels
    UrlCopyOpts opts;
    opts.parse(argc, argv);
    setupLogging(opts.debugLevel);

    // Construct Url Copy Process
    LegacyReporter reporter(opts);
    UrlCopyProcess urlCopyProcess(opts, reporter);

    // Set up signal handler
    panic::setup_signal_handlers(signalCallback, &urlCopyProcess);

    // Run the transfer
    try {
        urlCopyProcess.run();
    }
    catch (const std::exception &e) {
        urlCopyProcess.panic(e.what());
    }

    return 0;
}
