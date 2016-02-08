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

#ifndef FTS3_REPORTER_H
#define FTS3_REPORTER_H

#include <string>
#include "msg-bus/producer.h"

#include "Gfal2.h"
#include "Transfer.h"
#include "UrlCopyOpts.h"


/// Virtual interface for reporting transfer statuses back to FTS
class Reporter {
public:
    virtual ~Reporter();

    /// Notify the start of the transfer process
    virtual void sendTransferStart(const Transfer&, Gfal2TransferParams&) = 0;

    /// Notify which protocol settings are going to be used.
    /// This can not be sent with the start message since we need to do first a stat on the source
    /// to get the size, and then decide number of streams, buffersize, ...
    virtual void sendProtocol(const Transfer&, Gfal2TransferParams&) = 0;

    /// Notify the completion of the transfer
    virtual void sendTransferCompleted(const Transfer&, Gfal2TransferParams&) = 0;

    /// Periodic notifications
    virtual void sendPing(const Transfer&) = 0;
};

/// Implements reporter using MsgBus and newer message types
class DefaultReporter: public Reporter {
private:
    Producer producer;
    UrlCopyOpts opts;

public:
    DefaultReporter(const UrlCopyOpts &opts);

    virtual void sendTransferStart(const Transfer&, Gfal2TransferParams&);

    virtual void sendProtocol(const Transfer&, Gfal2TransferParams&);

    virtual void sendTransferCompleted(const Transfer&, Gfal2TransferParams&);

    virtual void sendPing(const Transfer&);
};

#endif // FTS3_REPORTER_H
