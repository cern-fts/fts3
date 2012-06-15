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
 * @file SeProtocolConfig.h
 * @brief model SeProtocolConfig
 * @author Michail Salichos
 * 
 **/


#pragma once

#include <iostream>

class SeProtocolConfig {
public:

    SeProtocolConfig() {
        SE_ROW_ID = 0;
        BANDWIDTH = 0;
        NOSTREAMS = 0;
        NOFILES = 0;
        TCP_BUFFER_SIZE = 0;
        NOMINAL_THROUGHPUT = 0;
        SE_LIMIT = 0;
        BLOCKSIZE = 0;
        HTTP_TO = 0;
        URLCOPY_PUT_TO = 0;
        URLCOPY_PUTDONE_TO = 0;
        URLCOPY_GET_TO = 0;
        URLCOPY_GETDONE_TO = 0;
        URLCOPY_TX_TO = 0;
        URLCOPY_TXMARKS_TO = 0;
        SRMCOPY_TO = 0;
        SRMCOPY_REFRESH_TO = 0;
        URL_COPY_FIRST_TXMARK_TO = 0;
        TX_TO_PER_MB = 0;
        NO_TX_ACTIVITY_TO = 0;
        PREPARING_FILES_RATIO = 0;


    }

    ~SeProtocolConfig() {
    }

    int SE_ROW_ID; //				   NOT NULL NUMBER(38)
    std::string SE_GROUP_NAME; //					    VARCHAR2(50)
    std::string SOURCE_SE; //					    VARCHAR2(512)
    std::string DEST_SE; //					    VARCHAR2(512)
    std::string CONTACT; //					    VARCHAR2(255)
    int BANDWIDTH; //					    NUMBER
    int NOSTREAMS; //					    NUMBER(38)
    int NOFILES; //					    NUMBER(38)
    int TCP_BUFFER_SIZE; //				    NUMBER
    int NOMINAL_THROUGHPUT; //				    NUMBER
    std::string SE_PAIR_STATE; //					    VARCHAR2(30)
    time_t LAST_ACTIVE; //					    TIMESTAMP(6) WITH TIME ZONE
    std::string MESSAGE; //					    VARCHAR2(1024)
    time_t LAST_MODIFICATION; //				    TIMESTAMP(6) WITH TIME ZONE
    std::string ADMIN_DN; //					    VARCHAR2(1024)
    int SE_LIMIT; //					    NUMBER(38)
    int BLOCKSIZE; //					    NUMBER
    int HTTP_TO; //					    NUMBER(38)
    std::string TX_LOGLEVEL; //					    VARCHAR2(12)
    int URLCOPY_PUT_TO; // 				    NUMBER(38)
    int URLCOPY_PUTDONE_TO; //				    NUMBER(38)
    int URLCOPY_GET_TO; // 				    NUMBER(38)
    int URLCOPY_GETDONE_TO; //				    NUMBER(38)
    int URLCOPY_TX_TO; //					    NUMBER(38)
    int URLCOPY_TXMARKS_TO; //				    NUMBER(38)
    std::string SRMCOPY_DIRECTION; //				    VARCHAR2(4)
    int SRMCOPY_TO; //					    NUMBER(38)
    int SRMCOPY_REFRESH_TO; //				    NUMBER(38)
    std::string TARGET_DIR_CHECK; //				    CHAR(1)
    int URL_COPY_FIRST_TXMARK_TO; //			    NUMBER(38)
    int TX_TO_PER_MB; //					    NUMBER
    int NO_TX_ACTIVITY_TO; //				    NUMBER(38)
    int PREPARING_FILES_RATIO; //				    NUMBER

};
