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
#include <sstream> //to phrase string
#include <string>
#include <map>
#include <boost/lexical_cast.hpp>
#include "msg-bus/producer.h"

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
message broker config file, retrieve info and store in STL map
 */
bool get_mon_cfg_file(const std::string &configFile);

/*get the values from the config file to getter functions*/
std::string getBROKER();
std::string getSTART();
std::string getCOMPLETE();
std::string getSTATE();
std::string getTTL();
bool getTOPIC();
bool getACTIVE();
std::string getLOGFILEDIR();
std::string getLOGFILENAME();
std::string getUSERNAME();
std::string getPASSWORD();
bool getUSE_BROKER_CREDENTIALS();


/*convert string to int safely*/
int GetIntVal(std::string strConvert);


/*remove any non-ascii characters from a string*/
std::string ReplaceNonPrintableCharacters(const std::string &s);

int restoreMessageToDisk(Producer &producer, const std::string &text);


#endif // UTILITYROUTINES_H
