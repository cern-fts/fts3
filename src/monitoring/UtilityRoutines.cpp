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

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <cctype> //for isdigit
#include <sstream> //to phrase std::string
#include <sys/stat.h>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iomanip>
#include <fstream>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <boost/lexical_cast.hpp>

#include <string>
#include <vector>

#include "common/definitions.h"
#include "common/DaemonTools.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "msg-bus/producer.h"
#include "UtilityRoutines.h"

#define MILLI 36000000

const std::string DATABASE_FILE_NAME = ".properties.xml";
const std::string DATABASE_FILE_PATH = "/etc/glite-data-transfer-agents.d/";

std::string BROKER;
std::string START;
std::string COMPLETE;
std::string STATE;
std::string TTL;
bool TOPIC;
bool ACTIVE;
std::string LOGFILEDIR;
std::string LOGFILENAME;
std::string USERNAME;
std::string PASSWORD;
bool USE_BROKER_CREDENTIALS;


//checks if a std::string is full of digits
bool isDigits(std::string word)
{
    for (unsigned int i = 0; i < word.size(); ++i) {
        if (!isdigit(word[i])) return false;
    }
    return true;
}


std::string strip_space(const std::string &s)
{
    std::string ret(s);
    while (ret.length() && (ret[0] == ' ' || ret[0] == '\t'))
        ret = ret.substr(1);
    while (ret.length() && (ret[ret.length() - 1] == ' ' || ret[ret.length() - 1] == '\t'))
        ret = ret.substr(0, ret.length() - 1);
    return ret;
}


std::string getBROKER()
{
    return BROKER;
}


std::string getSTART()
{
    return START;
}


std::string getCOMPLETE()
{
    return COMPLETE;
}


std::string getSTATE()
{
    return STATE;
}


std::string getTTL()
{
    return TTL;
}


bool getTOPIC()
{
    return TOPIC;
}


bool getACTIVE()
{
    return ACTIVE;
}


std::string getLOGFILEDIR()
{
    return LOGFILEDIR;
}


std::string getLOGFILENAME()
{
    return LOGFILENAME;
}


std::string getUSERNAME()
{
    return USERNAME;
}


std::string getPASSWORD()
{
    return PASSWORD;
}


bool getUSE_BROKER_CREDENTIALS()
{
    return USE_BROKER_CREDENTIALS;
}


/// message broker config file
bool get_mon_cfg_file(const std::string &configFile)
{
    std::map<std::string, std::string> cfg;

    std::string boolTOPIC;
    std::string boolACTIVE;
    std::string boolENABLELOG;
    std::string boolENABLEMSGLOG;
    std::string boolUSE_BROKER_CREDENTIALS;
    std::string filename;

    try {
        if (configFile.length() == 0)
            return false;

        std::ifstream in(configFile);
        if (!in) {
            FTS3_COMMON_LOGGER_LOG(CRIT, "msg config file cannot be read, check location and permissions");
            return false;
        }

        cfg.clear();
        std::string line;
        while (!in.eof()) {
            getline(in, line);
            line = strip_space(line);
            if (line.length() && line[0] != '#') {
                size_t pos = line.find("=");
                if (pos != std::string::npos) {
                    std::string key = strip_space(line.substr(0, pos));
                    std::string value = strip_space(line.substr(pos + 1));
                    cfg.insert(make_pair(key, value));
                }
            }
        }

        auto iter1 = cfg.find("BROKER");
        auto iter2 = cfg.find("START");
        auto iter3 = cfg.find("COMPLETE");
        auto iter6 = cfg.find("TTL");
        auto iter5 = cfg.find("TOPIC");
        auto iter7 = cfg.find("ACTIVE");
        auto iter9 = cfg.find("LOGFILEDIR");
        auto iter10 = cfg.find("LOGFILENAME");
        auto iter11 = cfg.find("FQDN");
        auto iter13 = cfg.find("USERNAME");
        auto iter14 = cfg.find("PASSWORD");
        auto iter15 = cfg.find("USE_BROKER_CREDENTIALS");
        auto iter16 = cfg.find("STATE");


        if (iter16 != cfg.end()) {
            STATE = cfg.find("STATE")->second;
            if (STATE.length() == 0)
                STATE = "";
        }

        if (iter13 != cfg.end()) {
            USERNAME = cfg.find("USERNAME")->second;
            if (USERNAME.length() == 0)
                USERNAME = "";
        }

        if (iter14 != cfg.end()) {
            PASSWORD = cfg.find("PASSWORD")->second;
            if (PASSWORD.length() == 0)
                PASSWORD = "";
        }


        if (iter15 != cfg.end()) {
            boolUSE_BROKER_CREDENTIALS = cfg.find("USE_BROKER_CREDENTIALS")->second;
            if (boolUSE_BROKER_CREDENTIALS.compare("true") == 0)
                USE_BROKER_CREDENTIALS = true;
            else
                USE_BROKER_CREDENTIALS = false;
        }

        if (iter9 != cfg.end()) {
            LOGFILEDIR = cfg.find("LOGFILEDIR")->second;
            if (LOGFILEDIR[LOGFILEDIR.length() - 1] != '/')
                LOGFILEDIR += "/";
        }
        else {
            LOGFILEDIR = "/var/log/glite/";
        }

        if (iter10 != cfg.end())
            LOGFILENAME = cfg.find("LOGFILENAME")->second;
        else
            LOGFILENAME = "msg.log";

        if (iter1 != cfg.end())
            BROKER = cfg.find("BROKER")->second;
        if (iter2 != cfg.end())
            START = cfg.find("START")->second;
        if (iter3 != cfg.end())
            COMPLETE = cfg.find("COMPLETE")->second;
        if (iter6 != cfg.end())
            TTL = cfg.find("TTL")->second;

        if (iter5 != cfg.end()) {
            boolTOPIC = cfg.find("TOPIC")->second;
            if (boolTOPIC.compare("true") == 0)
                TOPIC = true;
            else
                TOPIC = false;
        }
        if (iter7 != cfg.end()) {
            boolACTIVE = cfg.find("ACTIVE")->second;
            if (boolACTIVE.compare("true") == 0)
                ACTIVE = true;
            else
                ACTIVE = false;
        }

        if (USERNAME == "replacethis" || PASSWORD == "replacethis")
            throw fts3::common::UserError("Can not start with the default configuration");

        return true;
    }
    catch (const std::exception &e) {
        FTS3_COMMON_LOGGER_LOG(ERR, std::string("msg config file error ") + e.what());
        return false;
    }
    catch (...) {
        FTS3_COMMON_LOGGER_LOG(ERR, std::string("msg config file error: unexpected exception"));
        return false;
    }
}


int GetIntVal(std::string strConvert)
{
    try {
        if (isDigits(strConvert))
            return (atoi(strConvert.c_str())) * MILLI;

        return MILLI; //default value is ten hours
    }
    catch (...) {
        return MILLI; //default value is ten hours
    }

}


std::string ReplaceNonPrintableCharacters(const std::string &s)
{
    std::string result;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        int AsciiValue = static_cast<int> (c);
        if (AsciiValue < 32 || AsciiValue > 127) {
            result.append(" ");
        }
        else {
            result += s.at(i);
        }
    }
    return result;
}


int restoreMessageToDisk(Producer &producer, const std::string &text)
{
    struct MessageMonitoring message;
    g_strlcpy(message.msg, text.c_str(), sizeof(message.msg));
    message.timestamp = milliseconds_since_epoch();
    return producer.runProducerMonitoring(message);
}
