/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 ***********************************************/


#pragma once

#include <iostream>
#include <ctime> 
#include "parse_url.h"

class FileManagement {
public:
    FileManagement();
    ~FileManagement();

public:
    void archive();
    std::string generateLogFileName(std::string surl, std::string durl, std::string & file_id, std::string & job_id);
    bool directoryExists(const char* pzPath);
    std::string timestamp();
    void getLogStream(std::ofstream& logStream);
    void setSourceUrl(std::string& source_url);
    void setDestUrl(std::string& dest_url);
    void setFileId(std::string& file_id);
    void setJobId(std::string& job_id);
    std::string getLogFileName(){
    	return fname;
    }
    std::string getSePair(){
    	std::string pair = shost + "__" + dhost;
    	return pair;
    }
    std::string getSourceHostname(){
    	return shost;
    }
    std::string getDestHostname(){
    	return dhost;
    }

private:
    std::string source_url;
    std::string dest_url;
    std::string file_id;
    std::string job_id;
    std::string logFileName;
    std::string archiveFileName;
    std::string fname;
    std::string shost;
    std::string dhost;
};
