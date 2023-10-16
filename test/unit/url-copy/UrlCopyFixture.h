/*
 * Copyright (c) CERN 2023
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

#pragma once

#include "url-copy/UrlCopyProcess.h"

class UrlCopyFixture: public Reporter {
protected:
    UrlCopyOpts opts;
    std::list<Transfer> completedMsgs, startMsgs, pingMsgs, protoMsgs;

public:
    UrlCopyFixture() {
        opts.logToStderr = false;
        opts.logDir = "/tmp/fts3-tests";
    }

    void sendTransferStart(const Transfer &t, Gfal2TransferParams&) {
        startMsgs.push_back(t);
    }

    void sendProtocol(const Transfer &t, Gfal2TransferParams&) {
        protoMsgs.push_back(t);
    }

    void sendTransferCompleted(const Transfer &t, Gfal2TransferParams&) {
        completedMsgs.push_back(t);
    }

    void sendPing(Transfer &t) {
        pingMsgs.push_back(t);
    }
};
