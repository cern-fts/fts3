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

#include <iomanip>
#include <sstream>
#include "Transfer.h"


Transfer::Transfer() : fileId(0), userFileSize(0), scitag(0),
                       isMultipleReplicaJob(false), isLastReplica(false),
                       isMultihopJob(false), isLastHop(false), isArchiving(false),
                       checksumMode(Transfer::CHECKSUM_NONE), fileSize(0),
                       averageThroughput(0.0), instantaneousThroughput(0.0),
                       transferredBytes(0), previousPingTransferredBytes(0)
{
}


double Transfer::getTransferDurationInSeconds() const
{
    if (stats.transfer.start == 0 || stats.transfer.end == 0) {
        return 0;
    }
    return (static_cast<double>(stats.transfer.end - stats.transfer.start)) / 1000.0;
}


std::string Transfer::getTransferId() const
{
    time_t current;
    time(&current);
    struct tm * date = gmtime(&current);

    std::stringstream ss;
    ss << std::setfill('0')
    << std::setw(4) << (date->tm_year + 1900)
    << "-" << std::setw(2) << (date->tm_mon + 1)
    << "-" << std::setw(2) << (date->tm_mday)
    << "-" << std::setw(2) << (date->tm_hour)
    << std::setw(2) << (date->tm_min)
    << "__" << source.host << "__" << destination.host << "__" << fileId << "__" << jobId;

    return ss.str();
}


std::string Transfer::getChannel() const
{
    std::stringstream str;
    str << source.host << "__" << destination.host;
    return str.str();
}


std::ostream& operator << (std::ostream& out, const Transfer::Checksum_mode& c)
{
    switch (c) {
        case Transfer::CHECKSUM_NONE:
            out << "No checksum comparison";
            break;
        case Transfer::CHECKSUM_BOTH:
            out << "Both checksum comparison";
            break;
        case Transfer::CHECKSUM_TARGET:
            out << "Target comparison";
            break;
        case Transfer::CHECKSUM_SOURCE:
            out << "Source comparison";
            break;
        default:
            out << "Unknown value!";
    }
    return out;
}
