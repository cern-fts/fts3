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

#include "common/Uri.h"
#include "UrlCopyError.h"

class UrlCopyOpts;
using fts3::common::Uri;


class Transfer
{
public:
    typedef std::list<Transfer> TransferList;

    enum class IPver {
        UNKNOWN,
        IPv4,
        IPv6
    };

    // Transform IPver enum to 'ipver' keyword ("ipv6" | "ipv4" | "unknown")
    static std::string IPverToIPv6String(const IPver ipver) {
        if (ipver == IPver::UNKNOWN) {
            return "unknown";
        } else if (ipver == IPver::IPv6) {
            return "ipv6";
        } else {
            return "ipv4";
        }
    }

    struct Statistics {
        struct Interval {
            uint64_t start, end;
            Interval(): start(0), end(0) {}
        };

        Interval transfer;
        Interval sourceChecksum;
        Interval destChecksum;
        Interval srmPreparation;
        Interval srmFinalization;

        Interval process;

        ///< Flag for IP version used during transfer
        IPver ipver;
        ///< Eviction return code: 0 = ok, > 0 error code, -1 = not set
        int32_t evictionRetc;

        std::string finalDestination;
        std::string transferType;

        Statistics(): ipver(IPver::UNKNOWN), evictionRetc(-1) {};
    };

    /**
     * Checksum verification mode
     */
    typedef enum {
        /// Don't verify checksum
        CHECKSUM_NONE    = 0x00,
        /// Compare user provided checksum vs source
        CHECKSUM_SOURCE  = 0x01,
        /// Compare user provided checksum vs destination
        CHECKSUM_TARGET  = 0x02,
        /// Compare user provided checksum vs both, *or* source checksum vs target checksum
        CHECKSUM_BOTH = (CHECKSUM_SOURCE | CHECKSUM_TARGET)
    } Checksum_mode;


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
    std::string sourceTokenIssuer;
    std::string destTokenIssuer;
    bool        isMultipleReplicaJob;
    bool        isLastReplica;
    bool        isMultihopJob;
    bool        isLastHop;

    Checksum_mode checksumMode;

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

std::ostream& operator << (std::ostream& out, const Transfer::Checksum_mode& c);

#endif // TRANSFER_H_
