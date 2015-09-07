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

/**
 * Describes the status of one file in a transfer job.
 */
class TransferFile
{
public:


    TransferFile() :
            fileId(0), fileIndex(0), filesize(0.0), pinLifetime(0),
            bringOnline(0), userFilesize(0.0), lastReplica(0)
    {
    }

    ~TransferFile()
    {
    }

    int fileId;
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
    double filesize;
    time_t finishTime;
    std::string internalFileParams;
    time_t jobFinished;
    std::string voName;
    std::string overwriteFlag;
    std::string userDn;
    std::string credId;
    std::string checksumMethod;
    std::string checksum;
    std::string sourceSpaceToken;
    std::string destinationSpaceToken;
    std::string selectionStrategy;
    int pinLifetime;
    int bringOnline;
    double userFilesize;
    std::string fileMetadata;
    std::string jobMetadata;
    std::string bringOnlineToken;
    std::string userCredentials;
    std::string reuseJob;
    int lastReplica;
};

#endif // TRANSFERFILES_H_
