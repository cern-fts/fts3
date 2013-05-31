/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***********************************************/

/**
 * @file FileTransferStatus.h
 * @brief file transfer status object model
 * @author Michail Salichos
 * @date 09/02/2012
 *
 **/


#pragma once

#include <iostream>

/**
 * Describes the status of one file in a transfer job.
 */
class TransferFiles
{
public:


    TransferFiles():FILE_ID(0),FILE_INDEX(0),JOB_ID(""),FILE_STATE(""),LOGICAL_NAME(""),SOURCE_SURL(""),SOURCE_SE(""),DEST_SE(""),DEST_SURL(""),AGENT_DN(""),ERROR_SCOPE(""),ERROR_PHASE(""),REASON_CLASS(""),REASON(""),NUM_FAILURES(""),CURRENT_FAILURES(""),CATALOG_FAILURES(""),PRESTAGE_FAILURES(""),FILESIZE(""),CHECKSUM(""),FINISH_TIME(""),INTERNAL_FILE_PARAMS(""),JOB_FINISHED(""),VO_NAME(""),OVERWRITE(""),DN(""),CRED_ID(""),CHECKSUM_METHOD(""),SOURCE_SPACE_TOKEN(""),DEST_SPACE_TOKEN(""),PIN_LIFETIME(0),BRINGONLINE(0),USER_FILESIZE(0.0),FILE_METADATA(""),JOB_METADATA(""),BRINGONLINE_TOKEN("")
    {
    }

    ~TransferFiles()
    {
    }

    int FILE_ID;
    int FILE_INDEX;
    std::string JOB_ID;
    std::string FILE_STATE;
    std::string LOGICAL_NAME;
    std::string SOURCE_SURL;
    std::string SOURCE_SE;
    std::string DEST_SE;
    std::string DEST_SURL;
    std::string AGENT_DN;
    std::string ERROR_SCOPE;
    std::string ERROR_PHASE;
    std::string REASON_CLASS;
    std::string REASON;
    std::string NUM_FAILURES;
    std::string CURRENT_FAILURES;
    std::string CATALOG_FAILURES;
    std::string PRESTAGE_FAILURES;
    std::string FILESIZE;
    std::string CHECKSUM;
    std::string FINISH_TIME;
    std::string INTERNAL_FILE_PARAMS;
    std::string JOB_FINISHED;
    std::string VO_NAME;
    std::string OVERWRITE;
    std::string DN;
    std::string CRED_ID;
    std::string CHECKSUM_METHOD;
    std::string SOURCE_SPACE_TOKEN;
    std::string DEST_SPACE_TOKEN;
    int PIN_LIFETIME;
    int BRINGONLINE;
    double USER_FILESIZE;
    std::string FILE_METADATA;
    std::string JOB_METADATA;
    std::string BRINGONLINE_TOKEN;
};
