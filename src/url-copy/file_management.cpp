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
#include <fstream>


using namespace std;



FileManagement::FileManagement() :file_id(0), logFileName("/var/log/fts3/"), base_scheme(NULL), base_host(NULL), base_path(NULL), base_port(0), castor(false)
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

std::string FileManagement::getLogFilePath()
{
    return logFileName + "/" + fname;
}

void FileManagement::setSourceUrl(std::string& source_url)
{
    this->source_url = source_url;
    //source
    parse_url(source_url.c_str(), &base_scheme, &base_host, &base_port, &base_path);
    if(base_scheme && base_host)
        {
            shost = std::string(base_scheme) + "://" + std::string(base_host);
            shostFile = std::string(base_host);
        }
    else
        {
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
    if(base_scheme && base_host)
        {
            dhost = std::string(base_scheme) + "://" + std::string(base_host);
            dhostFile = std::string(base_host);
        }
    else
        {
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

void FileManagement::setFileId(unsigned file_id)
{
    this->file_id = file_id;
}

void FileManagement::setJobId(std::string& job_id)
{
    this->job_id = job_id;
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

    // Move log
    int r = rename(getLogFilePath().c_str(), arcFileName.c_str());
    if (r != 0)
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

    // Move debug file
    std::string debugFile    = getLogFilePath() + ".debug";
    std::string debugArchive = arcFileName + ".debug";
    if (access(debugFile.c_str(), F_OK) == 0)
        {
            int r2 = rename(debugFile.c_str(), debugArchive.c_str());
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

std::string FileManagement::generateLogFileName(std::string, std::string, unsigned file_id, std::string & job_id)
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


