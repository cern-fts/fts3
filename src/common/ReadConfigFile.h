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


/**
 * @file ReadConfigFile.h
 * @brief read fts3 configuration file
 * @author Michail Salichos
 * @date 09/02/2012
 * 
 **/

#pragma once
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include "MutexLocker.h"

class ReadConfigFile {
public:

    ReadConfigFile();

    ~ReadConfigFile();

    std::string getDBLibName();
    std::string getDBConnectionString();
    std::string getDBUsername();
    std::string getDBPassword();

    static ReadConfigFile & instance() {
        if (i.get() == 0) {
            MutexLocker obtain_lock(m);
            if (i.get() == 0) {
                i.reset(new ReadConfigFile);
            }
        }
        return *i;
    }

    bool isDBCfgValid();

private:
    std::map<std::string, std::string> cfgFile;
    std::string username;
    std::string password;
    std::string connectString;
    std::string dbLibName;
    static std::auto_ptr<ReadConfigFile> i;
    static Mutex m;
};
