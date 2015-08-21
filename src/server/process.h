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

#include <string>
#include <map>
using namespace std;


class ExecuteProcess
{
public:
    ExecuteProcess( const string& app, const string& arguments);
    int executeProcessShell(std::string& forkMessage);

    inline int getPid()
    {
        return pid;
    }

protected:
    int execProcessShell(std::string& forkMessage);
    void getArgv(list<string>& argsHolder, size_t* argc, char*** argv);

private:
    int pid;
    string m_app;
    string m_arguments;
};
