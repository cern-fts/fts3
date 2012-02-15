
#include "ReadConfigFile.h"
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <cctype> 
#include <sstream> 
#include <string>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <map>
#include <memory>
#include "Logger.h"


#define CONFIG_FILE "/etc/fts3config"

using namespace std;

static string strip_space(const string & s) {
    string ret(s);
    while (ret.length() && (ret[0] == ' ' || ret[0] == '\t'))
        ret = ret.substr(1);
    while (ret.length() && (ret[ret.length() - 1] == ' ' || ret[ret.length() - 1] == '\t'))
        ret = ret.substr(0, ret.length() - 1);
    return ret;
}

std::auto_ptr<ReadConfigFile> ReadConfigFile::i;
Mutex ReadConfigFile::m;

// Implementation 

ReadConfigFile::ReadConfigFile() {
    ifstream in(CONFIG_FILE);
    if (!in) {
        Logger::instance().error("Configuration file cannot be found, check path and permissions");
        exit(1);
    }

    cfgFile.clear();
    string line;
    while (!in.eof()) {
        getline(in, line);
        line = strip_space(line);
        if (line.length() && line[0] != '#') {
            int pos = line.find("=");
            if (pos != string::npos) {
                string key = strip_space(line.substr(0, pos));
                string value = strip_space(line.substr(pos + 1));
                cfgFile.insert(make_pair(key, value));
            }
        }
    }

    map <string, string> ::const_iterator iterDBLIBNAME = cfgFile.find("DBLIBNAME");
    map <string, string> ::const_iterator iterDBUSERNAME = cfgFile.find("DBUSERNAME");
    map <string, string> ::const_iterator iterDBCONNSTRING = cfgFile.find("DBCONNSTRING");
    map <string, string> ::const_iterator iterDBPASSWORD = cfgFile.find("DBPASSWORD");

    if (iterDBUSERNAME != cfgFile.end()) {
        username = cfgFile.find("DBUSERNAME")->second;
        if (username.length() == 0)
            username = "";
    }

    if (iterDBLIBNAME != cfgFile.end()) {
        dbLibName = cfgFile.find("DBLIBNAME")->second;
        if (dbLibName.length() == 0)
            dbLibName = "";
    }

    if (iterDBCONNSTRING != cfgFile.end()) {
        connectString = cfgFile.find("DBCONNSTRING")->second;
        if (connectString.length() == 0)
            connectString = "";
    }

    if (iterDBPASSWORD != cfgFile.end()) {
        password = cfgFile.find("DBPASSWORD")->second;
        if (password.length() == 0)
            password = "";
    }

}

ReadConfigFile::~ReadConfigFile() {

}

bool ReadConfigFile::isDBCfgValid() {
    if (username.length() > 0 &&
            dbLibName.length() > 0 &&
            connectString.length() &&
            password.length() > 0)
        return true;

    Logger::instance().error("Configuration file contains invalid entries, check username, password, etc");
    exit(1);
    return false;

}

std::string ReadConfigFile::getDBLibName() {
    return dbLibName;
}

std::string ReadConfigFile::getDBConnectionString() {
    return connectString;
}

std::string ReadConfigFile::getDBUsername() {
    return username;
}

std::string ReadConfigFile::getDBPassword() {
    return password;
}
