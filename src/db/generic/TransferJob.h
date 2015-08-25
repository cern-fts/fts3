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
#include "OwnedResource.h"

/**
 * Describes the status of one file in a transfer job.
 */
class TransferJob: public OwnedResource
{
public:

    TransferJob() :
            SUBMIT_TIME(0), FINISH_TIME(0), PRIORITY(3), MAX_TIME_IN_QUEUE(0),
            JOB_FINISHED(0), COPY_PIN_LIFETIME(0), BRINGONLINE(0)
    {
    }

    std::string JOB_ID;//NOT NULL CHAR(36)
    std::string JOB_STATE;//				   NOT NULL VARCHAR2(32)
    std::string CANCEL_JOB;//					    CHAR(1)
    std::string JOB_PARAMS;//					    VARCHAR2(255)
    std::string SOURCE;// 					    VARCHAR2(255)
    std::string DEST;//						    VARCHAR2(255)
    std::string USER_DN;//				   NOT NULL VARCHAR2(1024)
    std::string AGENT_DN;//					    VARCHAR2(1024)
    std::string USER_CRED;//					    VARCHAR2(255)
    std::string CRED_ID;//					    VARCHAR2(100)
    std::string VOMS_CRED;//					    BLOB
    std::string VO_NAME;//					    VARCHAR2(50)
    std::string SE_PAIR_NAME;//					    VARCHAR2(32)
    std::string REASON;//					    VARCHAR2(1024)
    time_t SUBMIT_TIME;//					    TIMESTAMP(6) WITH TIME ZONE
    time_t FINISH_TIME;//					    TIMESTAMP(6) WITH TIME ZONE
    int PRIORITY;//					    NUMBER(38)
    std::string SUBMIT_HOST;//					    VARCHAR2(255)
    int MAX_TIME_IN_QUEUE;//					    NUMBER(38)
    std::string SPACE_TOKEN;//						    VARCHAR2(255)
    std::string STORAGE_CLASS;//						    VARCHAR2(255)
    std::string MYPROXY_SERVER;//	 				    VARCHAR2(255)
    std::string SRC_CATALOG;//						    VARCHAR2(1024)
    std::string SRC_CATALOG_TYPE;//					    VARCHAR2(1024)
    std::string DEST_CATALOG;//						    VARCHAR2(1024)
    std::string DEST_CATALOG_TYPE;//					    VARCHAR2(1024)
    std::string INTERNAL_JOB_PARAMS;//					    VARCHAR2(255)
    std::string OVERWRITE_FLAG;//	 				    CHAR(1)
    time_t JOB_FINISHED;//						    TIMESTAMP(6) WITH TIME ZONE
    std::string SOURCE_SPACE_TOKEN;//					    VARCHAR2(255)
    std::string SOURCE_TOKEN_DESCRIPTION;//				    VARCHAR2(255)
    int COPY_PIN_LIFETIME;//					    NUMBER(38)
    std::string LAN_CONNECTION;//	 				    CHAR(1)
    std::string FAIL_NEARLINE;//						    CHAR(1)
    std::string CHECKSUM_METHOD;//					    CHAR(1)
    int BRINGONLINE; //					    NUMBER(38)
    std::string REUSE;

    std::string getUserDn() const
    {
        return USER_DN;
    }
    std::string getVo() const
    {
        return VO_NAME;
    }
};
