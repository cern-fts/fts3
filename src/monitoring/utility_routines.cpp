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

#include "utility_routines.h"
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include<cctype> //for isdigit
#include<sstream> //to phrase std::string
#include<string>
#include "Logger.h"
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
#include "common/producer_consumer_common.h"
#include "common/name_to_uid.h"
#include "common/error.h"
#include <boost/lexical_cast.hpp>

#define MILLI 36000000

const std::string CFG_FILE_NAME = "fts-msg-monitoring.conf";
const std::string CFG_FILE_PATH = "/etc/fts3/";
const std::string DATABASE_FILE_NAME = ".properties.xml";
const std::string DATABASE_FILE_PATH = "/etc/glite-data-transfer-agents.d/";
const std::string TEMPLOG = "/var/log/fts3/msg.log";


std::string BROKER;
std::string START;
std::string COMPLETE;
std::string STATE;
std::string CRON;
std::string TTL;
bool TOPIC;
bool ACTIVE;
bool ENABLELOG;
std::string LOGFILEDIR;
std::string LOGFILENAME;
std::string CRONFQDN;
bool ENABLEMSGLOG;

std::string USERNAME;
std::string PASSWORD;
bool USE_BROKER_CREDENTIALS;

static bool init = false;

//oracle credentials
static std::vector<std::string> fileDB;

//msg config file
static std::map<std::string, std::string> cfg;

//recovery vector in case no data is retrieved
static std::vector<std::string> recoveryVector(3, "");

//store fts endpoint for usage in the message field
static std::map<std::string, std::string> ftsendpoint;


int fexists(const char *filename)
{
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

int getdir(std::string rootDir, std::vector<std::string> &files)
{
    DIR *dir; //the directory
    struct dirent *dp;

    //open the directory
    if ((dir = opendir(rootDir.c_str())) == NULL)
        {
            return errno;
        }

    while ((dp = readdir(dir)) != NULL)
        {
            files.push_back(std::string(dp->d_name));
        }

    closedir(dir);
    return 0;
}

std::string filesStore(const char* filename, const char *path, char *env)
{
    std::string dirname;
    std::string rootDir;
    std::vector<std::string> files = std::vector<std::string > ();
    std::string inventory[3] = {"", "/usr", "/opt/glite"};
    std::string xmlfile;
    size_t found;

    try
        {
            if (env == NULL)
                {
                    for (int i = 0; i < 3; ++i)
                        {
                            dirname = inventory[i];
                            dirname = dirname + path;
                            rootDir = std::string(dirname);
                            getdir(rootDir, files);

                            const std::string fname = filename;
                            if(true == caseInsCompare(fname, CFG_FILE_NAME))
                                {
                                    std::string fullPath = CFG_FILE_PATH + CFG_FILE_NAME;
                                    if(0 == fexists(fullPath.c_str()))
                                        {
                                            return fullPath;
                                        }
                                    else
                                        {
                                            logger::writeMsgNoConfig("/etc/fts3/fts-msg-monitoring.conf configuration file cannot be found");

                                        }
                                }
                            else
                                {
                                    for (unsigned int i = 0; i < files.size(); i++)
                                        {
                                            xmlfile = files[i];
                                            found = xmlfile.find(filename);
                                            if (found != std::string::npos)
                                                {
                                                    return dirname + xmlfile;
                                                }
                                        }
                                }
                        }
                }
            else
                {
                    dirname = env;
                    dirname = dirname + path;
                    rootDir = std::string(dirname);
                    getdir(rootDir, files);

                    for (unsigned int i = 0; i < files.size(); i++)
                        {
                            xmlfile = files[i];
                            found = xmlfile.find(filename);
                            if (found != std::string::npos)
                                {
                                    return dirname + xmlfile;
                                }
                        }
                    for (int i = 0; i < 3; ++i)
                        {
                            dirname = inventory[i];
                            dirname = dirname + path;
                            rootDir = std::string(dirname);
                            getdir(rootDir, files);

                            for (unsigned int i = 0; i < files.size(); i++)
                                {
                                    xmlfile = files[i];
                                    found = xmlfile.find(filename);
                                    if (found != std::string::npos)
                                        {
                                            return dirname + xmlfile;
                                        }
                                }
                        }
                }
            return "";
        }
    catch (...)
        {
            logger::writeLog("Configuration file cannot be found", true);
            return "";
        }
}


//checks if a std::string is full of digits
bool isDigits(std::string word)
{
    for (unsigned int i = 0; i < word.size(); ++i)
        {
            if (!isdigit(word[i])) return false;
        }
    return true;
}

std::string extractNumber(const std::string & value)
{
    std::string sentence = value;
    std::stringstream extract; // extract words by words;

    extract << sentence; //enter the sentence that we want to extract word by word

    std::string word = "";

    //while there are words to extract
    while (!extract.fail())
        {
            extract >> word; //extract the word

            if (isDigits(word))
                {
                    if (atoi(word.c_str()) > 399 && atoi(word.c_str()) < 554) //report only gridftp error codes
                        return word;
                }
        }
    return "";
}


std::string strip_space(const std::string & s)
{
    std::string ret(s);
    while (ret.length() && (ret[0] == ' ' || ret[0] == '\t'))
        ret = ret.substr(1);
    while (ret.length() && (ret[ret.length() - 1] == ' ' || ret[ret.length() - 1] == '\t'))
        ret = ret.substr(0, ret.length() - 1);
    return ret;
}


std::string getMsgConfigFile()
{
    std::string filename("");
    try
        {
            filename = filesStore(CFG_FILE_NAME.c_str(), CFG_FILE_PATH.c_str(), NULL);
            if (filename.length() > 0)
                return filename;
            else
                {
                    return std::string("");
                }

        }
    catch (...)
        {
            logger::writeMsgNoConfig("/etc/fts3/fts-msg-monitoring.conf file cannot be found");
            return std::string("");
        }
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


std::string getCRON()
{
    return CRON;
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

bool getENABLELOG()
{
    return ENABLELOG;
}

std::string getCRONFQDN()
{
    return CRONFQDN;
}

std::string getLOGFILEDIR()
{
    return LOGFILEDIR;
}

std::string getLOGFILENAME()
{
    return LOGFILENAME;
}

bool getENABLEMSGLOG()
{
    return ENABLEMSGLOG;
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


//message broker config file

bool get_mon_cfg_file()
{
    std::string boolTOPIC;
    std::string boolACTIVE;
    std::string boolENABLELOG;
    std::string boolENABLEMSGLOG;
    std::string boolUSE_BROKER_CREDENTIALS;
    std::string filename;

    try
        {
            filename = getMsgConfigFile();
            if (filename.length() == 0)
                return false;

            std::string ifname(filename);
            std::ifstream in(ifname.c_str());
            if(!in)
                {
                    logger::writeLog("msg config file cannot be read, check location and permissions", true);
                    return false;
                }

            cfg.clear();
            std::string line;
            while (!in.eof())
                {
                    getline(in, line);
                    line = strip_space(line);
                    if (line.length() && line[0] != '#')
                        {
                            size_t pos = line.find("=");
                            if (pos != std::string::npos)
                                {
                                    std::string key = strip_space(line.substr(0, pos));
                                    std::string value = strip_space(line.substr(pos + 1));
                                    cfg.insert(make_pair(key, value));
                                }
                        }
                }

            auto iter1 = cfg.find("BROKER");
            auto iter2 = cfg.find("START");
            auto iter3 = cfg.find("COMPLETE");
            auto iter4 = cfg.find("CRON");
            auto iter6 = cfg.find("TTL");
            auto iter5 = cfg.find("TOPIC");
            auto iter7 = cfg.find("ACTIVE");
            auto iter8 = cfg.find("ENABLELOG");
            auto iter9 = cfg.find("LOGFILEDIR");
            auto iter10 = cfg.find("LOGFILENAME");
            auto iter11 = cfg.find("FQDN");
            auto iter12 = cfg.find("ENABLEMSGLOG");
            auto iter13 = cfg.find("USERNAME");
            auto iter14 = cfg.find("PASSWORD");
            auto iter15 = cfg.find("USE_BROKER_CREDENTIALS");
            auto iter16 = cfg.find("STATE");


            if (iter16 != cfg.end())
                {
                    STATE = cfg.find("STATE")->second;
                    if (STATE.length() == 0)
                        STATE = "";
                }

            if (iter13 != cfg.end())
                {
                    USERNAME = cfg.find("USERNAME")->second;
                    if (USERNAME.length() == 0)
                        USERNAME = "";
                }

            if (iter14 != cfg.end())
                {
                    PASSWORD = cfg.find("PASSWORD")->second;
                    if (PASSWORD.length() == 0)
                        PASSWORD = "";
                }


            if (iter15 != cfg.end())
                {
                    boolUSE_BROKER_CREDENTIALS = cfg.find("USE_BROKER_CREDENTIALS")->second;
                    if (boolUSE_BROKER_CREDENTIALS.compare("true") == 0)
                        USE_BROKER_CREDENTIALS = true;
                    else
                        USE_BROKER_CREDENTIALS = false;
                }

            if (iter12 != cfg.end())
                {
                    boolENABLEMSGLOG = cfg.find("ENABLEMSGLOG")->second;
                    if (boolENABLEMSGLOG.compare("true") == 0)
                        ENABLEMSGLOG = true;
                    else
                        ENABLEMSGLOG = false;
                }

            if (iter11 != cfg.end())
                {
                    CRONFQDN = cfg.find("FQDN")->second;
                    if (CRONFQDN.length() == 0)
                        CRONFQDN = "";
                }

            if (iter9 != cfg.end())
                {
                    LOGFILEDIR = cfg.find("LOGFILEDIR")->second;
                    if( LOGFILEDIR[LOGFILEDIR.length()-1] != '/')
                        LOGFILEDIR += "/";
                }
            else
                {
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
            if (iter4 != cfg.end())
                CRON = cfg.find("CRON")->second;
            if (iter6 != cfg.end())
                TTL = cfg.find("TTL")->second;

            if (iter5 != cfg.end())
                {
                    boolTOPIC = cfg.find("TOPIC")->second;
                    if (boolTOPIC.compare("true") == 0)
                        TOPIC = true;
                    else
                        TOPIC = false;
                }
            if (iter7 != cfg.end())
                {
                    boolACTIVE = cfg.find("ACTIVE")->second;
                    if (boolACTIVE.compare("true") == 0)
                        ACTIVE = true;
                    else
                        ACTIVE = false;
                }
            if (iter8 != cfg.end())
                {
                    boolENABLELOG = cfg.find("ENABLELOG")->second;
                    if (boolENABLELOG.compare("true") == 0)
                        ENABLELOG = true;
                    else
                        ENABLELOG = false;
                }

            if (USERNAME == "replacethis" || PASSWORD == "replacethis" || CRONFQDN == "replacethis")
                throw fts3::common::Err_Custom("Can not start with the default configuration");

            return true;
        }
    catch (const std::exception& e)
        {
            LOGGER_ERROR(std::string("msg config file error " )+ e.what());
            return false;
        }
    catch (...)
        {
            LOGGER_ERROR(std::string("msg config file error: unexpected exception"));
            return false;
        }
}

void appendMessageToLogFile(std::string & text)
{
    static std::string filename = LOGFILEDIR + "" + LOGFILENAME;
    const char* logFileName = filename.c_str();
    static std::ofstream fout;
    uid_t pw_uid = name_to_uid();

    if(!init)
        {
            fout.open(filename.c_str(), std::ios::app); // open file for appending
            init = true;
        }
    if (fout.is_open()  && fexists(filename.c_str())==0)
        {
            fout << text << std::endl; //send to file

            if(fout.bad())
                {
                    fout.close();
                    init = false;
                }
        }
    else
        {
            fout.open(filename.c_str(), std::ios::app); // open file for appending
            fout << text << std::endl; //send to file
            fout.close();
            init = false;
        }
    int chownExec = chown(logFileName, pw_uid, getgid());
    if(chownExec == -1)
        {
            //do nothing
        }

}


void appendMessageToLogFileNoConfig(std::string & text)
{
    static std::string filename = TEMPLOG;
    const char* logFileName = filename.c_str();
    static std::ofstream fout;
    uid_t pw_uid = name_to_uid();

    fout.open(filename.c_str(), std::ios::app); // open file for appending
    if (fout.is_open())
        {
            fout << text << std::endl; //send to file
        }
    fout.close(); //close file
    int chownExec = chown(logFileName, pw_uid, getgid());
    if(chownExec == -1)
        {
            //do nothing
        }
}



int GetIntVal(std::string strConvert)
{
    try
        {
            if (isDigits(strConvert))
                return (atoi(strConvert.c_str())) * MILLI;

            return MILLI; //default value is ten hours
        }
    catch (...)
        {
            return MILLI; //default value is ten hours
        }

}

std::string ReplaceNonPrintableCharacters(std::string s)
{
    try
        {
            std::string result;
            for (size_t i = 0; i < s.length(); i++)
                {
                    char c = s[i];
                    int AsciiValue = static_cast<int> (c);
                    if (AsciiValue < 32 || AsciiValue > 127)
                        {
                            result.append(" ");
                        }
                    else
                        {
                            result += s.at(i);
                        }
                }
            return result;
        }
    catch (...)
        {
            return s;
        }
}


bool caseInsCompare(const std::string& s1, const std::string& s2)
{
    return((s1.size( ) == s2.size( )) &&
           equal(s1.begin( ), s1.end( ), s2.begin( ), caseInsCharCompareN));
}


std::string restoreMessageToDisk(std::string & text)
{
    struct message_monitoring message;
    strncpy(message.msg, text.c_str(), sizeof(message.msg));
    message.msg[sizeof(message.msg) - 1] = '\0';
    message.timestamp = milliseconds_since_epoch();
    int returnValue = runProducerMonitoring(message);

    if(returnValue == 0)
        return std::string();
    else
        return boost::lexical_cast<std::string>(returnValue);
}
