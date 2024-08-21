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

#ifndef URLCOPYPROCESS_H
#define URLCOPYPROCESS_H

#include <boost/thread.hpp>
#include <gfal_api.h>

#include "Gfal2.h"
#include "Reporter.h"
#include "UrlCopyOpts.h"
#include "UrlCopyError.h"

/// To be called by gfal2 on transfer events
void eventCallback(const gfalt_event_t e, gpointer udata);

/// To be called by gfal2 when performance markers are received
void performanceCallback(gfalt_transfer_status_t h, const char*, const char*, gpointer udata);

/// Main class of fts_url_copy. Implements the transfer logic.
class UrlCopyProcess {
private:
    boost::mutex transfersMutex;

    UrlCopyOpts opts;
    Transfer::TransferList todoTransfers;
    Transfer::TransferList doneTransfers;

    Reporter &reporter;

    Gfal2 gfal2;
    bool canceled;
    bool timeoutExpired;

    /// Run a single transfer
    void runTransfer(Transfer &transfer, Gfal2TransferParams &params);

    /// Archive the transfer logs
    void archiveLogs(Transfer &transfer);

    /// Transfer utils
    void cleanup_on_failure(Gfal2TransferParams &params, const std::string &destination);

public:

    /// Constructor. Initialize all internals from the command line options.
    UrlCopyProcess(const UrlCopyOpts &opts, Reporter &reporter);

    /// Run the UrlCopy process
    void run(void);

    /// Cancel gracefully the process: cancel the running transfer, and send cancellation
    /// messages for the remaining ones.
    void cancel(void);

    /// Send a termination messages for running and pending transfers. This is to be called
    /// just before a panic quit (i.e. from a SIGSEGV)
    void panic(const std::string &msg);

    /// Trigger a cancel, mark running transfer as expired.
    void timeout(void);
};


#endif // URLCOPYPROCESS_H
