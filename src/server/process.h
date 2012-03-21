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

#pragma once

#include <string>
using namespace std;


class ExecuteProcess
{
public:
    ExecuteProcess( const string& app, const string& arguments, int fdlog=0 );
    int executeProcess();
    int executeProcessShell();
    std::string generate_request_id(const std::string& prefix);

private:
    
    int execProcess(const int argc, char** argv);
    int execProcessLog(const int argc, char** argv);
    
    int execProcessShell(const char* shell);
    int execProcessShellLog(const char* shell);
    
    string m_app;
    string m_arguments;
    int m_fdlog;
};

