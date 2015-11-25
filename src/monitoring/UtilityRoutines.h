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
#ifndef UTILITYROUTINES_H
#define UTILITYROUTINES_H

#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cctype> //for isdigit
#include <sstream> //to phrase string
#include <string>
#include <map>

#include <boost/lexical_cast.hpp>

inline std::string getTimestampStr()
{
    std::chrono::milliseconds timestamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()
            );

    long double timestamp_double = static_cast<long double>(timestamp.count());
    std::string transfer_started_to_string = boost::lexical_cast<std::string>(timestamp_double);
    return transfer_started_to_string;
}

/*
extract a number from a given string
used for retrieving the error code of a failed transfer
 */
std::string extractNumber(const std::string & value);


/*
message broker config file, retrieve info and store in STL map
 */
bool get_mon_cfg_file();

/*get the values from the config file to getter functions*/
std::string getBROKER();
std::string getSTART();
std::string getCOMPLETE();
std::string getSTATE();
std::string getCRON();
std::string getTTL();
std::string getCRONFQDN();
bool getTOPIC();
bool getACTIVE();
bool getENABLELOG();
std::string getLOGFILEDIR();
std::string getLOGFILENAME();
bool getENABLEMSGLOG();
std::string getUSERNAME();
std::string getPASSWORD();
bool getUSE_BROKER_CREDENTIALS();


/*convert string to int safely*/
int GetIntVal(std::string strConvert);


/*remove any non-ascii characters from a string*/
std::string ReplaceNonPrintableCharacters(std::string s);

inline bool caseInsCharCompareN(char a, char b)
{
    return(toupper(a) == toupper(b));
}

bool caseInsCompare(const std::string& s1, const std::string& s2);

std::string restoreMessageToDisk(std::string & text);


//get timestamp in calendar time
inline std::string  timestamp()
{
    time_t ltime; /* calendar time */
    ltime=time(NULL); /* get current cal time */
    return asctime( localtime(&ltime));
}

#endif // UTILITYROUTINES_H
