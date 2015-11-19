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

#include <iostream>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common/Uri.h"

class FileManagement
{
public:
    FileManagement();
    ~FileManagement();

public:
    void init(std::string logDir);
    std::string archive();
    std::string generateLogFileName(std::string surl, std::string durl, unsigned file_id, std::string & job_id);
    static std::string dateDir();
    void generateLogFile();

    static bool directoryExists(const char* pzPath);

    std::string getLogFilePath();

    void setSourceUrl(std::string& source_url);
    void setDestUrl(std::string& dest_url);

    void setFileId(unsigned file_id);
    void setJobId(std::string& job_id);

    inline std::string getLogFileName()
    {
        return fname;
    }

    inline std::string getSePair()
    {
        std::string pair = shost + "__" + dhost;
        return pair;
    }

    inline std::string getSourceHostname()
    {
        return shost;
    }

    inline std::string getDestHostname()
    {
        return dhost;
    }

    inline std::string getSourceHostnameFile()
    {
        return shostFile;
    }

    inline std::string getDestHostnameFile()
    {
        return dhostFile;
    }

    inline std::string _getLogArchivedFileFullPath()
    {
        return arcFileName;
    }


private:
    std::string source_url;
    std::string dest_url;
    unsigned    file_id;
    std::string job_id;
    std::string logFileName;
    std::string archiveFileName;
    std::string fname;
    std::string shost;
    std::string dhost;
    std::string shostFile;
    std::string dhostFile;
    std::string dateArch;
    std::string arcFileName;
};
