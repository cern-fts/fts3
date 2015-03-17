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
 * various utility routines to be used across libraries and executables
 */

#include "utility_routines.h"
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include<cctype> //for isdigit
#include<sstream> //to phrase string
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
#include "producer_consumer_common.h"
#include "name_to_uid.h"
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
static map<string, string> cfg;

//recovery vector in case no data is retrieved
static vector<std::string> recoveryVector(3, "");

//store fts endpoint for usage in the message field
static map<string, string> ftsendpoint;



std::string get_channel_(std::string tr_id)
{
    size_t found = tr_id.find("__");
    if (found != string::npos)
        return tr_id.substr(0, (found));

    return tr_id;
}

std::string get_hostname(std::string hostname)
{
    size_t found = 0;
    size_t found_colon = 0;
    size_t found_extra_slash = 0; //for gsiftp only

    if (hostname.compare(0, 9, "gsiftp://") == 0)
        {
            std::string temp = hostname.substr(9, found - 9);
            found = temp.find_first_of('/');
            if (found != string::npos)
                {
                    found_extra_slash = temp.find_first_of('/');
                    if (found_extra_slash != string::npos)
                        return temp.substr(0, found_extra_slash);
                    found_colon = temp.find_first_of(':');
                    if (found_colon != string::npos)
                        return temp.substr(0, found_colon);
                    else
                        return temp.substr(0, found);
                }
            else
                {
                    found = temp.find_first_of(':');
                    if (found != string::npos)
                        return temp.substr(0, found);
                }
        }


    if (hostname.compare(0, 6, "srm://") == 0)
        {
            std::string temp = hostname.substr(6, found - 6);
            found = temp.find_first_of('/');
            if (found != string::npos)
                {
                    found_colon = temp.find_first_of(':');
                    if (found_colon != string::npos)
                        return temp.substr(0, found_colon);
                    else
                        return temp.substr(0, found);
                }
            else
                {
                    found = temp.find_first_of(':');
                    if (found != string::npos)
                        return temp.substr(0, found);
                }
        }
    return "invalid hostname";
}

std::string getUserName(std::string & value, std::vector<std::string>::iterator it)
{
    size_t found = value.find("User");
    std::string username;
    if (found != std::string::npos)
        {
            username = *(++it);
            return username.substr(7, (username.length() - 15));
        }
    return "";
}

std::string getPassword(std::string & value, std::vector<std::string>::iterator it)
{
    size_t found = value.find("Password");
    std::string password;
    if (found != std::string::npos)
        {
            password = *(++it);
            return password.substr(7, (password.length() - 15));
        }
    return "";
}

std::string getConnectString(std::string & value, std::vector<std::string>::iterator it)
{
    size_t found = value.find("ConnectString");
    std::string connectstring("");
    std::string connectStringConcat("");
    size_t val;
    string::iterator itr;
    if (found != std::string::npos)
        {
            connectstring = *(++it);
            val = connectstring.find("</value>");
            if (val != std::string::npos)
                return connectstring.substr(7, (connectstring.length() - 15));
            else
                {
                    --it;
                    while( 1 )
                        {
                            connectStringConcat += *(++it);
                            val = connectStringConcat.find("</value>");
                            if (val != std::string::npos)
                                {
                                    connectStringConcat.erase(0,7);
                                    connectStringConcat.erase(connectStringConcat.length()-8, connectStringConcat.length());
                                    break;
                                }
                        }
                }
        }
    return connectStringConcat;
}

int fexists(const char *filename)
{
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

int getdir(string rootDir, vector<string> &files)
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
            files.push_back(string(dp->d_name));
        }

    closedir(dir);
    return 0;
}

std::string filesStore(const char* filename, const char *path, char *env)
{
    std::string dirname;
    std::string rootDir;
    vector<std::string> files = vector<std::string > ();
    string inventory[3] = {"", "/usr", "/opt/glite"};
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
                                    if (found!=string::npos)
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

std::string getGliteLocationFile()
{
    std::string filename;
    char * pPath = NULL;
    try
        {
            pPath = getenv("GLITE_LOCATION");
            if (pPath != NULL)
                {
                    filename = filesStore(DATABASE_FILE_NAME.c_str(), DATABASE_FILE_PATH.c_str(), pPath);
                    if (filename.length() > 0)
                        return filename;
                    else
                        {
                            logger::writeLog("GLITE_LOCATION is set, but *.properties file cannot be found under " + DATABASE_FILE_PATH, true);
                            return "";
                        }

                }
            else
                {
                    filename = filesStore(DATABASE_FILE_NAME.c_str(), DATABASE_FILE_PATH.c_str(), NULL);
                    if (filename.length() > 0)
                        return filename;
                    else
                        {
                            logger::writeLog("GLITE_LOCATION is not set, and not other location contains the *.properties file", true);
                            return "";
                        }
                }
        }
    catch (...)
        {
            logger::writeLog("Database credentials file cannot be found, pls check FTS database *.properties file location", true);
            return "";
        }
}

std::vector<std::string> const& oracleCredentials()
{
    std::string word;
    std::string username = "";
    std::string password = "";
    std::string connectstring = "";
    bool userOk = false, passOk = false, connectOk = false;
    std::string filename;

    try
        {
            filename = getGliteLocationFile();
            if (filename.length() == 0)
                return recoveryVector;

            std::ifstream in(filename.c_str());
            if(!in)
                {
                    logger::writeLog("Database credentials file cannot be read, check location and permissions", true);
                    return recoveryVector;
                }

            std::vector<std::string>::iterator it;

            while (in >> word)
                {
                    fileDB.push_back(word);
                }

            for (it = fileDB.begin(); it < fileDB.end(); it++)
                {
                    if (!userOk)
                        {
                            username = getUserName(*it, it);
                            if (username.length() > 0)
                                {
                                    userOk = true;
                                    continue;
                                }
                        }

                    if (!passOk)
                        {
                            password = getPassword(*it, it);
                            if (password.length() > 0)
                                {
                                    passOk = true;
                                    continue;
                                }
                        }

                    if (!connectOk)
                        {
                            connectstring = getConnectString(*it, it);
                            if (connectstring.length() > 0)
                                {
                                    connectOk = true;
                                    continue;
                                }
                        }
                }

            in.close();
            fileDB.clear();
            fileDB.push_back(username);
            fileDB.push_back(password);
            fileDB.push_back(connectstring);

            return fileDB;
        }
    catch (...)
        {
            logger::writeLog("Database credentials file cannot be found", true);
            return recoveryVector;
        }
}

//checks if a string is full of digits

bool isDigits(string word)
{
    for (unsigned int i = 0; i < word.size(); ++i)
        {
            if (!isdigit(word[i])) return false;
        }
    return true;
}

std::string extractNumber(const std::string & value)
{
    string sentence = value;
    stringstream extract; // extract words by words;

    extract << sentence; //enter the sentence that we want to extract word by word

    string word = "";

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

string strip_space(const string & s)
{
    string ret(s);
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

            string ifname(filename);
            ifstream in(ifname.c_str());
            if(!in)
                {
                    logger::writeLog("msg config file cannot be read, check location and permissions", true);
                    return false;
                }

            cfg.clear();
            string line;
            while (!in.eof())
                {
                    getline(in, line);
                    line = strip_space(line);
                    if (line.length() && line[0] != '#')
                        {
                            size_t pos = line.find("=");
                            if (pos != string::npos)
                                {
                                    string key = strip_space(line.substr(0, pos));
                                    string value = strip_space(line.substr(pos + 1));
                                    cfg.insert(make_pair(key, value));
                                }
                        }
                }

            map <string, string> ::const_iterator iter1 = cfg.find("BROKER");
            map <string, string> ::const_iterator iter2 = cfg.find("START");
            map <string, string> ::const_iterator iter3 = cfg.find("COMPLETE");
            map <string, string> ::const_iterator iter4 = cfg.find("CRON");
            map <string, string> ::const_iterator iter6 = cfg.find("TTL");
            map <string, string> ::const_iterator iter5 = cfg.find("TOPIC");
            map <string, string> ::const_iterator iter7 = cfg.find("ACTIVE");
            map <string, string> ::const_iterator iter8 = cfg.find("ENABLELOG");
            map <string, string> ::const_iterator iter9 = cfg.find("LOGFILEDIR");
            map <string, string> ::const_iterator iter10 = cfg.find("LOGFILENAME");
            map <string, string> ::const_iterator iter11 = cfg.find("FQDN");
            map <string, string> ::const_iterator iter12 = cfg.find("ENABLEMSGLOG");
            map <string, string> ::const_iterator iter13 = cfg.find("USERNAME");
            map <string, string> ::const_iterator iter14 = cfg.find("PASSWORD");
            map <string, string> ::const_iterator iter15 = cfg.find("USE_BROKER_CREDENTIALS");
            map <string, string> ::const_iterator iter16 = cfg.find("STATE");


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
            logger::writeLog(std::string("msg config file error: ") + e.what(), true);
            return false;
        }
    catch (...)
        {
            logger::writeLog("msg config file error", true);
            return false;
        }
}

void appendMessageToLogFile(std::string & text)
{
    static std::string filename = LOGFILEDIR + "" + LOGFILENAME;
    const char* logFileName = filename.c_str();
    static ofstream fout;
    uid_t pw_uid = name_to_uid();

    if(!init)
        {
            fout.open(filename.c_str(), ios::app); // open file for appending
            init = true;
        }
    if (fout.is_open()  && fexists(filename.c_str())==0)
        {
            fout << text << endl; //send to file

            if(fout.bad())
                {
                    fout.close();
                    init = false;
                }
        }
    else
        {
            fout.open(filename.c_str(), ios::app); // open file for appending
            fout << text << endl; //send to file
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
    static ofstream fout;
    uid_t pw_uid = name_to_uid();

    fout.open(filename.c_str(), ios::app); // open file for appending
    if (fout.is_open())
        {
            fout << text << endl; //send to file
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

std::string ReplaceNonPrintableCharacters(string s)
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


bool caseInsCompare(const string& s1, const string& s2)
{
    return((s1.size( ) == s2.size( )) &&
           equal(s1.begin( ), s1.end( ), s2.begin( ), caseInsCharCompareN));
}


std::string send_message(std::string & text)
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


bool getResolveAlias()
{
    return true;
}

std::string getFTSEndpoint()
{
    const char* path[2] = {"/etc/glite-sd2cache-cron.conf","/opt/glite/etc/glite-sd2cache-cron.conf"};
    string line;
    vector<std::string>::iterator myVectorIterator;
    std::string fts = "";
    std::string result = "";

    for(int index=0; index<2; index++)
        {
            string ifname(path[index]);
            ifstream in(ifname.c_str());
            if(!in)
                {
                    continue;
                }

            string line;
            while (!in.eof())
                {
                    getline(in, line);
                    line = strip_space(line);
                    if (line.length() && line[0] != '#')
                        {
                            size_t pos = line.find("=");
                            if (pos != string::npos)
                                {
                                    string key = strip_space(line.substr(0, pos));
                                    string value = strip_space(line.substr(pos + 1));
                                    ftsendpoint.insert(make_pair(key, value));
                                }
                        }
                }

            map <string, string> ::const_iterator iter = ftsendpoint.find("FTS_HOST");
            if (iter != ftsendpoint.end())
                {
                    fts = ftsendpoint.find("FTS_HOST")->second;
                    if (fts.length() == 0)
                        fts = "";
                }
        }
    if(fts.length() > 0)
        {
            fts.erase(0, 1);
            fts.erase(fts.length()-1, fts.length());
            result = "https://";
            result += fts;
            result += ":8443/glite-data-transfer-fts/services/FileTransfer";
        }
    return result;
}
