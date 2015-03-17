/**
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://www.eu-egee.org/partners/ for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

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

using namespace std;

/*
convert a number to string, given the base
 */
template <class T>
std::string _to_string(T t, std::ios_base & (*)(std::ios_base&))
{
    std::ostringstream oss;
    oss << fixed << t;
    return oss.str();
}

inline std::string _getTimestamp()
{
    time_t msec_started = time(NULL) * 1000; //the number of milliseconds since the epoch.
    std::string transfer_started_to_string = _to_string<long double>(msec_started, std::dec);
    return transfer_started_to_string;
}


/*
extract channel information from the transfer id
 */
std::string get_channel_(std::string tr_id);


/*
get transfer agent hostname
 */
std::string get_hostname(std::string hostname);

/*
retrieve database username from the config xml file
 */
std::string getUserName(std::string & value, std::vector<std::string>::iterator it);

/*
retrieve database password from the config xml file
 */
std::string getPassword(std::string & value, std::vector<std::string>::iterator it);

/*
retrieve database connection string from the config xml file
 */
std::string getConnectString(std::string & value, std::vector<std::string>::iterator it);

/*
retrieve glite location from GLITE_LOCATION env variable
 */
std::string getGliteLocationFile();

/*
store database connection credentials to an STL vector
 */
std::vector<std::string> const& oracleCredentials();

/*
extract a number from a given string
used for retrieving the error code of a failed transfer
 */
std::string extractNumber(const std::string & value);


/*
get timestamp in epoc-millisecconds
 */
std::string _getTimestamp();


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


/*
append the content of the message to a log file
*/
void appendMessageToLogFile(std::string & text);

/*
append error message to a default log file
*/
void appendMessageToLogFileNoConfig(std::string & text);


/*convert string to int safely*/
int GetIntVal(std::string strConvert);


/*remove any non-ascii characters from a string*/
std::string ReplaceNonPrintableCharacters(std::string s);

inline bool caseInsCharCompareN(char a, char b)
{
    return(toupper(a) == toupper(b));
}

bool caseInsCompare(const string& s1, const string& s2);

std::string send_message(std::string & text);


//get timestamp in calendar time
inline std::string  timestamp()
{
    time_t ltime; /* calendar time */
    ltime=time(NULL); /* get current cal time */
    return asctime( localtime(&ltime));
}


bool getResolveAlias();

std::string getFTSEndpoint();

