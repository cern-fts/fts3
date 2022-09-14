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

#include <gfal_api.h>
#include <boost/algorithm/string.hpp>
#include "common/Logger.h"
#include "Transfer.h"

using fts3::common::commit;

static const GQuark GFAL_GRIDFTP_PASV_STAGE_QUARK = g_quark_from_static_string("PASV");


void performanceCallback(gfalt_transfer_status_t h, const char*, const char*, gpointer udata)
{
    if (h) {
        Transfer *transfer = (Transfer*)(udata);

        size_t avg = gfalt_copy_get_average_baudrate(h, NULL);
        if (avg > 0) {
            avg = avg / 1024;
        }
        else {
            avg = 0;
        }
        size_t inst = gfalt_copy_get_instant_baudrate(h, NULL);
        if (inst > 0) {
            inst = inst / 1024;
        }
        else {
            inst = 0;
        }

        size_t trans = gfalt_copy_get_bytes_transfered(h, NULL);
        time_t elapsed = gfalt_copy_get_elapsed_time(h, NULL);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "bytes: " << trans
            << ", avg KB/sec:" << avg
            << ", inst KB/sec:" << inst
            << ", elapsed:" << elapsed
            << commit;
        transfer->throughput = (double) avg;
        transfer->transferredBytes = trans;
    }
}

/// gfal2 GridFTP plugin gives us source and destination urls as
/// (source-ip) source => (destination-ip) destination, so extract source and destination
/// turls from there
static void extractTurls(Transfer *transfer, std::string str)
{
    std::string delimiter = "=>";
    size_t pos = 0;
    std::string token;

    std::size_t begin = str.find_first_of("(");
    std::size_t end = str.find_first_of(")");
    if (std::string::npos != begin && std::string::npos != end && begin <= end) {
        str.erase(begin, end - begin + 1);
    }

    std::size_t begin2 = str.find_first_of("(");
    std::size_t end2 = str.find_first_of(")");
    if (std::string::npos != begin2 && std::string::npos != end2 && begin2 <= end2) {
        str.erase(begin2, end2 - begin2 + 1);
    }

    while ((pos = str.find(delimiter)) != std::string::npos) {
        token = str.substr(0, pos);
        std::string sourceTurl = token;
        str.erase(0, pos + delimiter.length());
        std::string destinationTurl = str;

        boost::trim(sourceTurl);
        boost::trim(destinationTurl);

        transfer->sourceTurl = Uri::parse(sourceTurl);
        transfer->destTurl = Uri::parse(destinationTurl);
    }
}


void eventCallback(const gfalt_event_t e, gpointer udata)
{
    static const char *sideStr[] = {"SOURCE", "DEST", "BOTH"};
    static const GQuark SRM_DOMAIN = g_quark_from_static_string("SRM");

    Transfer *transfer = (Transfer*)(udata);


    FTS3_COMMON_LOGGER_NEWLOG(INFO) << '[' << e->timestamp << "] "
        << sideStr[e->side] << ' '
        << g_quark_to_string(e->domain) << '\t'
        << g_quark_to_string(e->stage) << '\t'
        << e->description
    << commit;

    std::string &source = transfer->source.fullUri;
    std::string &dest = transfer->destination.fullUri;

    if (e->stage == GFAL_EVENT_TRANSFER_ENTER && ((source.compare(0, 6, "srm://") == 0) || (dest.compare(0, 6, "srm://") == 0))) {
        extractTurls(transfer, e->description);
    }
    if (e->stage == GFAL_EVENT_TRANSFER_ENTER) {
        transfer->stats.transfer.start = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_TRANSFER_EXIT) {
        transfer->stats.transfer.end = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_CHECKSUM_ENTER && e->side == GFAL_EVENT_SOURCE) {
        transfer->stats.sourceChecksum.start = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_CHECKSUM_EXIT && e->side == GFAL_EVENT_SOURCE) {
        transfer->stats.sourceChecksum.end = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_CHECKSUM_ENTER && e->side == GFAL_EVENT_DESTINATION) {
        transfer->stats.destChecksum.start = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_CHECKSUM_EXIT && e->side == GFAL_EVENT_DESTINATION) {
        transfer->stats.destChecksum.end = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_PREPARE_ENTER && e->domain == SRM_DOMAIN) {
        transfer->stats.srmPreparation.start = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_PREPARE_EXIT && e->domain == SRM_DOMAIN) {
        transfer->stats.srmPreparation.end = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_CLOSE_ENTER && e->domain == SRM_DOMAIN) {
        transfer->stats.srmFinalization.start = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_CLOSE_EXIT && e->domain == SRM_DOMAIN) {
        transfer->stats.srmFinalization.end = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_IPV4) {
        transfer->stats.ipver = Transfer::IPver::IPv4;
    }
    else if (e->stage == GFAL_EVENT_IPV6) {
        transfer->stats.ipver = Transfer::IPver::IPv6;
    }
    else if (e->stage == GFAL_EVENT_EVICT) {
        try {
            transfer->stats.evictionRetc = std::abs(std::stoi(e->description));
        } catch(...) {
            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Invalid eviction return code received: " << e->description << commit;
        }
    }
    else if (e->stage == GFAL_EVENT_TRANSFER_TYPE) {
        transfer->stats.transferType = e->description;
    }
    else if (e->stage== GFAL_GRIDFTP_PASV_STAGE_QUARK) {
        transfer->stats.finalDestination = e->description;
    }
}
