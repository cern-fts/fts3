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
#ifndef TRANSFERFILES_H_
#define TRANSFERFILES_H_

#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/logic/tribool.hpp>

#include "Job.h"

enum class CopyMode {
    ANY,
    PULL,
    PUSH,
    STREAMING,
};

/**
 * Describes the status of one file in a transfer job.
 */
class TransferFile
{
public:

    struct ProtocolParameters {
        int nostreams;
        int timeout;
        int buffersize;
        bool strictCopy;
        boost::tribool ipv6;
        boost::tribool udt;
        // If true, gfal2/davix must generate the S3 signature with the bucket on the path
        bool s3Alternate;

        ProtocolParameters(): nostreams(1), timeout(0), buffersize(0),
          strictCopy(false), s3Alternate(false) {}

        ProtocolParameters(const std::string &serialized): nostreams(1), timeout(0), buffersize(0),
            strictCopy(false), ipv6(boost::indeterminate), udt(boost::indeterminate), s3Alternate(false)
        {
            std::vector<std::string> params;
            boost::split(params, serialized, boost::is_any_of(","));

            for (auto i = params.begin(); i != params.end(); ++i) {
                if (boost::starts_with(*i, "nostreams:")) {
                    nostreams = boost::lexical_cast<int>(i->substr(10));
                }
                else if (boost::starts_with(*i, "timeout:")) {
                    timeout = boost::lexical_cast<int>(i->substr(8));
                }
                else if (boost::starts_with(*i, "buffersize:")) {
                    buffersize = boost::lexical_cast<int>(i->substr(11));
                }
                else if (*i == "strict") {
                    strictCopy = true;
                }
                else if (*i == "ipv4") {
                    ipv6 = false;
                }
                else if (*i == "ipv6") {
                    ipv6 = true;
                }
                else if (*i == "s3alternate") {
                    s3Alternate = true;
                }
            }
        }
    };


    TransferFile() :
            fileId(0), fileIndex(0), numFailures(0), filesize(0.0), userFilesize(0.0),
            finishTime(0), jobFinished(0), pinLifetime(0), bringOnline(0), archiveTimeout(0),
            scitag(0), jobType(Job::kTypeRegular), lastReplica(0), lastHop(0), pid(0)
    {
    }

    ~TransferFile()
    {
    }

    uint64_t fileId;
    int fileIndex;
    std::string jobId;
    std::string fileState;
    std::string sourceSurl;
    std::string sourceSe;
    std::string destSe;
    std::string destSurl;
    std::string agentDn;
    std::string reason;
    unsigned numFailures;
    int64_t filesize;
    int64_t userFilesize;
    time_t finishTime;
    std::string internalFileParams;
    time_t jobFinished;
    std::string voName;
    std::string overwriteFlag;
    std::string dstFileReport;
    std::string userDn;
    std::string credId;
    std::string sourceTokenId;
    std::string destinationTokenId;
    std::string checksumMode;
    std::string checksum;
    std::string sourceSpaceToken;
    std::string destinationSpaceToken;
    std::string selectionStrategy;
    std::string activity;
    int pinLifetime;
    int bringOnline;
    int archiveTimeout;
    int scitag;
    std::string fileMetadata;
    std::string jobMetadata;
    std::string archiveMetadata;
    std::string bringOnlineToken;
    Job::JobType jobType; // See Job constants
    int lastReplica;
    int lastHop;
    pid_t pid;

    ProtocolParameters getProtocolParameters(void) const {
        return ProtocolParameters(internalFileParams);
    }
};

#endif // TRANSFERFILES_H_
