#include "file_management.h"
#include "config/serverconfig.h"
#include <iostream>
#include <ctype.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>
#include <fstream>
#include <iomanip>
#include <sys/dir.h>

using namespace FTS3_CONFIG_NAMESPACE;
using namespace std;


FileManagement::FileManagement() {
    //logFileName = theServerConfig().get<std::string >("TransferLogDirectory");
    logFileName = "/var/log/fts3";    
    if (logFileName.length() > 0)
        directoryExists(logFileName.c_str());
}

FileManagement::~FileManagement() {
    //archive();
}

void FileManagement::getLogStream(std::ofstream& logStream) {
    fname = generateLogFileName(source_url, dest_url, file_id, job_id);
    logFileName += "/" + fname;
    logStream.open(logFileName.c_str(), ios::app);
}

void FileManagement::setSourceUrl(std::string& source_url) {
    this->source_url = source_url;
}

void FileManagement::setDestUrl(std::string& dest_url) {
    this->dest_url = dest_url;
}

void FileManagement::setFileId(std::string& file_id) {
    this->file_id = file_id;
}

void FileManagement::setJobId(std::string& job_id) {
    this->job_id = job_id;
}


//get timestamp in calendar time

std::string FileManagement::timestamp() {
    std::string timestapStr("");
    time_t ltime; /* calendar time */
    ltime = time(NULL); /* get current cal time */
    timestapStr = asctime(localtime(&ltime));
    timestapStr.erase(timestapStr.end() - 1);
    return timestapStr + " ";
}

bool FileManagement::directoryExists(const char* pzPath) {
    if (pzPath == NULL) return false;

    DIR *pDir;
    bool bExists = false;

    pDir = opendir(pzPath);

    if (pDir != NULL) {
        bExists = true;
        (void) closedir(pDir);
    } else {
        /* Directory does not exist */
        if (mkdir(pzPath, 0777) != 0)
            bExists = false;
    }

    return bExists;
}

void FileManagement::archive() {

    //archiveFileName: src__dest
    //: full path to file
    std::string arcFileName = "/var/log/fts3/" + archiveFileName;
    directoryExists(arcFileName.c_str());
    arcFileName += "/" + fname; 
    rename(logFileName.c_str(), arcFileName.c_str());
}

std::string FileManagement::generateLogFileName(std::string surl, std::string durl, std::string & file_id, std::string & job_id) {
    std::string new_name = std::string("");
    char *base_scheme = NULL;
    char *base_host = NULL;
    char *base_path = NULL;
    int base_port = 0;

    //add surl / durl
    //source
    parse_url(surl.c_str(), &base_scheme, &base_host, &base_port, &base_path);
    shost = base_host;

    //dest
    parse_url(durl.c_str(), &base_scheme, &base_host, &base_port, &base_path);
    dhost = base_host;

    archiveFileName = shost + "__" + dhost;

    // add date
    time_t current;
    time(&current);
    struct tm * date = gmtime(&current);

    // Create template
    std::stringstream ss;
    ss << std::setfill('0');
    ss << std::setw(4) << (date->tm_year + 1900)
            << "-" << std::setw(2) << (date->tm_mon + 1)
            << "-" << std::setw(2) << (date->tm_mday)
            << "-" << std::setw(2) << (date->tm_hour)
            << std::setw(2) << (date->tm_min)
            << "__" << shost << "__" << dhost << "__" << file_id << "__" << job_id;

    new_name += ss.str();
    return new_name;
}

