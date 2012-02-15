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
class FileTransferStatus {
public:

    FileTransferStatus() {
    }

    ~FileTransferStatus() {
    }

    /**
     * The logical name of the file being transfered (only used in the FPS).
     */
    std::string logicalName;

    /**
     * The source file for the transfer.
     */
    std::string sourceSURL;

    /**
     * The destination file for the transfer.
     */
    std::string destSURL;

    /**
     * The current state of the file transfer.
     */
    std::string transferFileState;

    /**
     * Number of times this transfer has failed (and retried).
     */
    int numFailures;

    /**
     * Small explanation on why it went wrong.
     */
    std::string reason;

    /**
     * Type of error.
     */
    std::string reason_class;

    /**
     * Transfer duration
     */
    long duration;

    std::string error_scope;

    std::string error_phase;

};
