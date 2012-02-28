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

/** \file main.cpp FTS3 server entry point. */

#include "server_dev.h"

#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "common/error.h"
#include "common/logger.h"
#include "config/serverconfig.h"

#include "server.h"
#include "daemonize.h"

using namespace FTS3_SERVER_NAMESPACE;
using namespace FTS3_COMMON_NAMESPACE;

/* -------------------------------------------------------------------------- */

/// Handler of SIGCHLD
void _handle_sigint(int)
{
    theServer().stop();
    kill(getpid(), 9);
    exit(EXIT_SUCCESS);
}

/* -------------------------------------------------------------------------- */

int main (int argc, char** argv)
{
    try 
    {
        FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv);
        struct sigaction action;
        action.sa_handler = _handle_sigint;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        int res = sigaction(SIGINT, &action, NULL);
            
        if (res == -1) 
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_System());
        }

        bool isDaemon = ! FTS3_CONFIG_NAMESPACE::theServerConfig().get<bool> ("no-daemon");

        if (isDaemon)
        {
            std::cout << "Going to daemon mode... by by console!" << std::endl;
            daemonize();
        }
        
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting server..." << commit;
        theServer().start();
    }
    catch(Err e)
    {
        std::string msg = "Unhandled exception, exiting...";
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
        return EXIT_FAILURE;
    }
    catch(...)
    {
        std::string msg = "Unknown exception, exiting...";
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

