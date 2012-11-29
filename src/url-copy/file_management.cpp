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
#include <errno.h>

using namespace FTS3_CONFIG_NAMESPACE;
using namespace std;

static int fexists(const char *filename) {
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

FileManagement::FileManagement() : base_scheme(NULL), base_host(NULL), base_path(NULL), base_port(0) {
    try {
        FTS3_CONFIG_NAMESPACE::theServerConfig().read(0, NULL);
        logFileName = theServerConfig().get<std::string > ("TransferLogDirectory");
        bdii = theServerConfig().get<std::string > ("Infosys");
        if (logFileName.length() > 0)
            directoryExists(logFileName.c_str());

        //generate arc based on date
        std::string dateArch = logFileName + "/" + dateDir();
        directoryExists(dateArch.c_str());
        logFileName = dateArch;
    } catch (...) {
        /*try again before let it fail*/
        try {
            FTS3_CONFIG_NAMESPACE::theServerConfig().read(0, NULL);
            logFileName = theServerConfig().get<std::string > ("TransferLogDirectory");
            bdii = theServerConfig().get<std::string > ("Infosys");
            if (logFileName.length() > 0)
                directoryExists(logFileName.c_str());
            //generate arc based on date
            std::string dateArch = logFileName + "/" + dateDir();
            directoryExists(dateArch.c_str());
            logFileName = dateArch;
        } catch (...) {
            /*no way to recover if an exception is thrown here, better let it fail and log the error*/
        }
    }
}

FileManagement::~FileManagement() {
}

void FileManagement::generateLogFile() {
    fname = generateLogFileName(source_url, dest_url, file_id, job_id);
}

int FileManagement::getLogStream(std::ofstream& logStream) {
    log = logFileName + "/" + fname;
    fullPath = log + ".debug";
    logStream.open(log.c_str(), ios::app);
    if (logStream.fail()) {
        return errno;
    } else {
        return 0;
    }
}

void FileManagement::setSourceUrl(std::string& source_url) {
    this->source_url = source_url;
    //source
    parse_url(source_url.c_str(), &base_scheme, &base_host, &base_port, &base_path);
    shost = std::string(base_host);
    if (base_scheme)
        free(base_scheme);
    if (base_host)
        free(base_host);
    if (base_path)
        free(base_path);
}

void FileManagement::setDestUrl(std::string& dest_url) {
    this->dest_url = dest_url;
    //dest
    parse_url(dest_url.c_str(), &base_scheme, &base_host, &base_port, &base_path);
    dhost = std::string(base_host);
    if (base_scheme)
        free(base_scheme);
    if (base_host)
        free(base_host);
    if (base_path)
        free(base_path);
}

void FileManagement::setFileId(std::string& file_id) {
    this->file_id = file_id;
}

void FileManagement::setJobId(std::string& job_id) {
    this->job_id = job_id;
}

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
        umask(0);
        if (mkdir(pzPath, S_IRWXU | S_IRWXG | S_IRWXO) != 0)
            bExists = false;
    }

    return bExists;
}

std::string FileManagement::archive() {
    char buf[256] = {0};
    std::string arcFileName = logFileName + "/" + archiveFileName;
    directoryExists(arcFileName.c_str());
    arcFileName += "/" + fname;
    int r = rename(log.c_str(), arcFileName.c_str());
    if (r == 0) {
        if (fexists(fullPath.c_str()) == 0) {
            std::string debugArchFile = arcFileName + ".debug";
            int r2 = rename(fullPath.c_str(), debugArchFile.c_str());
            if (r2 != 0) {
                char const * str = strerror_r(errno, buf, 256);
                if (str) {
                    return std::string(str);
                } else {
                    return std::string("Unknown error when moving debug log file");
                }
            }
        }
    } else {
        char const * str = strerror_r(errno, buf, 256);
        if (str) {
            return std::string(str);
        } else {
            return std::string("Unknown error when moving log file");
        }
    }
    return std::string("");
}

std::string FileManagement::dateDir() {
    // add date
    time_t current;
    time(&current);
    struct tm * date = gmtime(&current);

    // Create template
    std::stringstream ss;
    ss << std::setfill('0');
    ss << std::setw(4) << (date->tm_year + 1900)
            << "-" << std::setw(2) << (date->tm_mon + 1)
            << "-" << std::setw(2) << (date->tm_mday);

    return ss.str();
}

std::string FileManagement::generateLogFileName(std::string, std::string, std::string & file_id, std::string & job_id) {
    std::string new_name = std::string("");
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

