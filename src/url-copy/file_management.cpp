/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "file_management.h"
#include <iostream>
#include <ctype.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <sstream>
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


using namespace std;

static int fexists(const char *filename)
{
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

FileManagement::FileManagement() : logFileName("/var/log/fts3/"), base_scheme(NULL), base_host(NULL), base_path(NULL), base_port(0)
{
    try
        {
            if (logFileName.length() > 0)
                directoryExists(logFileName.c_str());

            //generate arc based on date
            std::string dateArch = logFileName + "/" + dateDir();
            directoryExists(dateArch.c_str());
            logFileName = dateArch;
        }
    catch (...)     //try again
        {
            if (logFileName.length() > 0)
                directoryExists(logFileName.c_str());

            //generate arc based on date
            std::string dateArch = logFileName + "/" + dateDir();
            directoryExists(dateArch.c_str());
            logFileName = dateArch;
        }
}

FileManagement::~FileManagement()
{
}

void FileManagement::generateLogFile()
{
    fname = generateLogFileName(source_url, dest_url, file_id, job_id);
}

int FileManagement::getLogStream(std::ofstream& logStream)
{
    log = logFileName + "/" + fname;
    fullPath = log + ".debug";
    logStream.open(log.c_str(), ios::app);
    if (logStream.fail())
        {
            return errno;
        }
    else
        {
            chmod(log.c_str(), (mode_t) 0644);
            return 0;
        }
}

void FileManagement::setSourceUrl(std::string& source_url)
{
    this->source_url = source_url;
    //source
    parse_url(source_url.c_str(), &base_scheme, &base_host, &base_port, &base_path);
    if(base_scheme && base_host){
    	shost = std::string(base_scheme) + "://" + std::string(base_host);
    	shostFile = std::string(base_host);
    }else{
    	shost = std::string("invalid") + "://" + std::string("invalid");
    	shostFile = std::string("invalid");    
    }    
    if (base_scheme)
        free(base_scheme);
    if (base_host)
        free(base_host);
    if (base_path)
        free(base_path);
}

void FileManagement::setDestUrl(std::string& dest_url)
{
    this->dest_url = dest_url;
    //dest
    parse_url(dest_url.c_str(), &base_scheme, &base_host, &base_port, &base_path);
    if(base_scheme && base_host){
    	dhost = std::string(base_scheme) + "://" + std::string(base_host);
    	dhostFile = std::string(base_host);
    }else{
        dhost = std::string("invalid") + "://" + std::string("invalid");
    	dhostFile = std::string("invalid");
    }
    if (base_scheme)
        free(base_scheme);
    if (base_host)
        free(base_host);
    if (base_path)
        free(base_path);
}

void FileManagement::setFileId(std::string& file_id)
{
    this->file_id = file_id;
}

void FileManagement::setJobId(std::string& job_id)
{
    this->job_id = job_id;
}

std::string FileManagement::timestamp()
{
    std::string timestapStr("");
    char timebuf[128] = "";
    // Get Current Time
    time_t current;
    time(&current);
    struct tm local_tm;
    localtime_r(&current, &local_tm);
    timestapStr = std::string(asctime_r(&local_tm, timebuf));
    timestapStr.erase(timestapStr.end() - 1);
    return timestapStr + " ";
}

bool FileManagement::directoryExists(const char* pzPath)
{
    if (pzPath == NULL) return false;

    DIR *pDir=NULL;
    bool bExists = false;

    pDir = opendir(pzPath);

    if (pDir != NULL)
        {
            bExists = true;
            (void) closedir(pDir);
        }
    else
        {
            /* Directory does not exist */
            umask(0);
            if (mkdir(pzPath, 0755) != 0)
                bExists = false;
        }

    return bExists;
}

std::string FileManagement::archive()
{
    char buf[256] = {0};
    arcFileName = logFileName + "/" + archiveFileName;
    directoryExists(arcFileName.c_str());
    arcFileName += "/" + fname;
    int r = rename(log.c_str(), arcFileName.c_str());
    if (r == 0)
        {
            if (fexists(fullPath.c_str()) == 0)
                {
                    std::string debugArchFile = arcFileName + ".debug";
                    int r2 = rename(fullPath.c_str(), debugArchFile.c_str());
                    if (r2 != 0)
                        {
                            char const * str = strerror_r(errno, buf, 256);
                            if (str)
                                {
                                    return std::string(str);
                                }
                            else
                                {
                                    return std::string("Unknown error when moving debug log file");
                                }
                        }
                }
        }
    else
        {
            char const * str = strerror_r(errno, buf, 256);
            if (str)
                {
                    return std::string(str);
                }
            else
                {
                    return std::string("Unknown error when moving log file");
                }
        }
    return std::string("");
}

std::string FileManagement::dateDir()
{
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

std::string FileManagement::generateLogFileName(std::string, std::string, std::string & file_id, std::string & job_id)
{
    std::string new_name = std::string("");
    archiveFileName = shostFile + "__" + dhostFile;

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
       << "__" << shostFile << "__" << dhostFile << "__" << file_id << "__" << job_id;

    new_name += ss.str();
    return new_name;
}

