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

#ifndef FTS3_LEGACYREPORTER_H
#define FTS3_LEGACYREPORTER_H

#include "Reporter.h"

/// Implements reporter using MsgBus
class LegacyReporter: public Reporter {
private:
    Producer producer;
    UrlCopyOpts opts;

public:
    LegacyReporter(const UrlCopyOpts &opts);

    virtual void sendTransferStart(const Transfer&, Gfal2TransferParams&);

    virtual void sendProtocol(const Transfer&, Gfal2TransferParams&);

    virtual void sendTransferCompleted(const Transfer&, Gfal2TransferParams&);

    virtual void sendPing(const Transfer&);
};

#endif // FTS3_LEGACYREPORTER_H
