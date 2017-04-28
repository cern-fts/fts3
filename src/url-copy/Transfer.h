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

#ifndef TRANSFER_H_
#define TRANSFER_H_

#include <cstdint>
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>

#include "common/definitions.h"
#include "common/Uri.h"
#include "UrlCopyError.h"

class UrlCopyOpts;
using fts3::common::Uri;


class Transfer
{
public:
    typedef std::list<Transfer> TransferList;

    struct Statistics {
        static uint64_t timestampMilliseconds()
        {
            return milliseconds_since_epoch();
        }

        struct Interval {
            uint64_t start, end;
        };

        Interval transfer;
        Interval sourceChecksum;
        Interval destChecksum;
        Interval srmPreparation;
        Interval srmFinalization;

        Interval process;

        bool ipv6Used;

        Statistics();
    };

    enum CompareChecksum
    {
        kChecksumDoNotCheck = 0, // Do not check checksum
        kChecksumStrict,         // Strict comparison
        kChecksumRelaxed         // Relaxed comparision. i.e. do not fail on empty checksum on source
    };

    std::string jobId;
    uint64_t    fileId;
    Uri         source;
    Uri         destination;

    Uri         sourceTurl;
    Uri         destTurl;

    std::string checksumAlgorithm;
    std::string checksumValue;
    uint64_t    userFileSize;
    std::string fileMetadata;
    std::string tokenBringOnline;
    std::string sourceTokenDescription;
    std::string destTokenDescription;
    bool        isMultipleReplicaJob;
    bool        isLastReplica;

    CompareChecksum checksumMethod;

    // File size
    uint64_t fileSize;

    // Progress markers
    double throughput;
    uint64_t transferredBytes;

    // Log file
    std::string logFile;
    std::string debugLogFile;

    // Bind error if any
    boost::shared_ptr<UrlCopyError> error;

    // Transfer statistics
    Statistics stats;

    Transfer();

    double getTransferDurationInSeconds() const;

    std::string getTransferId(void) const;

    std::string getChannel(void) const;
};

std::ostream& operator << (std::ostream& out, const Transfer::CompareChecksum& c);

#endif // TRANSFER_H_
