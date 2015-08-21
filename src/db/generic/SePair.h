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



#pragma once

#include <iostream>
#include <vector>
#include <map>


class SePair
{

public:
    SePair() {}
    ~SePair() {}

    /**
     * The name of the channel to select on
     */
    std::string sePairName;

    /**
     * The source site name of the channel. e.g. for CERN it should be "cern.ch".
     * In the case of a non-dedicated site to site link this should be "any".
     */
    std::string sourceSite;

    /**
     * The destination site of the channel. As for sourceSite.
     */
    std::string destSite;

    /**
     * The email contact for the responsbile of the channel.
     */
    std::string contact;

    /**
     * Number of concurrent streams allowed on the channel.
     */
    int numberOfStreams;

    /**
     * Number of concurrent files allowed on the channel.
     */
    int numberOfFiles;

    /**
     * Current maximum bandwidth (capacity) in Mbits/s.
     */
    int bandwidth;

    /**
     * The throughput goal for the system to achieve. Mbits/s.
     */
    int nominalThroughput;

    /**
     * The operational state of the channel.
     * "Active" means running and accepting jobs.
     * "Drain" means servicing jobs, but not accepting new ones from clients.
     * "Inactive" mean accepting new jobs from clients, but not servicing any new ones.
     * "Stopped" means neither accepting nor servicing jobs.
     */
    std::string state;

    /**
     * The transfer share for each VO on this channel {VOName,Share}.
     */
    std::map<std::string, std::string> VOShares;

    /** The message concerning the last modification. */
    std::string message;
    /** The time of the last modification. */
    time_t lastModificationTime;
    /** The DN of the last modifier. */
    std::string lastModifierDn;
    /** Default TCP buffer size for gridftp transfers. */
    std::string tcpBufferSize;
    /** A {VOName, int} array of VO limits. */
    std::map<std::string, int> VOLimits;
    /** Number of seconds to mark activity on URL Copy. */
    int urlCopyFirstTxmarkTo;
    /** Flag to check destination directory. */
    int targetDirCheck;

    /** The last time the channel was active. */
    time_t lastActiveTime;
    /** Don't use this. You have been warned! */
    int seLimit;
    /**
     * Type of a channel: DEDICATED (point-to-point,
     * seLimit is not considered)
     * or NONDEDICATED (start or cloud).
     */
    std::string channelType;
    /**
     * Default block size of a transfer, e.g. '1MB' or '1024kb'.
     * This parameter is passed to the globus FTP client.
     */
    std::string blockSize;
    /** Timeouts are in seconds. */
    int httpTimeout;
    /** Possible values are: DEBUG, INFO (default), ERROR. */
    std::string transferLogLevel;
    /** Ratio of SRM activity to number of transfer slots. */
    long preparingFilesRatio;

    /** Transfer type: URLCOPY|SRMCOPY. */
    std::string transferType;

    int urlCopyPutTimeout;
    int urlCopyPutDoneTimeout;
    int urlCopyGetTimeout;
    int urlCopyGetDoneTimeout;
    int urlCopyTransferTimeout;
    int urlCopyTransferMarkersTimeout;
    /** Make dCache happy timeout. */
    int urlCopyNoProgressTimeout;
    /** Transfer timeout per megabyte. */
    long urlCopyTransferTimeoutPerMB;

    /** SRM copy direction: PUSH, PULL. */
    std::string srmCopyDirection;
    /** This is a global timeout. */
    int srmCopyTimeout;
    /** Timeout between status updates. */
    int srmCopyRefreshTimeout;

};
