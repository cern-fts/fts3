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

#include <cstdio>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "common/error.h"
#include "common/logger.h"
#include "config/serverconfig.h"
#include "db/generic/SingleDbInstance.h"
#include "ws/GSoapDelegationHandler.h"
#include <fstream>
#include "server.h"
#include "daemonize.h"
#include "signal_logger.h"
#include "StaticSslLocking.h"


using namespace FTS3_SERVER_NAMESPACE;
using namespace FTS3_COMMON_NAMESPACE;

extern std::string stackTrace;
bool stopThreads = false;

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
static int fexists(const char *filename) {
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}


/// Handler of SIGCHLD
void _handle_sigint(int)
{
    if(stackTrace.length() > 0)
    	FTS3_COMMON_LOGGER_NEWLOG(ERR) << stackTrace << commit; 
    stopThreads = true;    
    theServer().stop();
    sleep(5);
    exit(EXIT_SUCCESS);
}

/* -------------------------------------------------------------------------- */

void fts3_initialize_db_backend()
{
    std::string dbUserName = theServerConfig().get<std::string>("DbUserName");
    std::string dbPassword = theServerConfig().get<std::string>("DbPassword");
    std::string dbConnectString = theServerConfig().get<std::string>("DbConnectString");
    
    try{
    	db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString);
    } 
    catch(Err_Custom& e)
    {            
        exit(1);
    }
    catch(...){
    	FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Something is going on with the database, check username/password/connstring" << commit;
	exit(1);
    }

   
}





static bool checkUrlCopy(){
        std::string p("");
        std::vector<std::string> pathV;
        std::vector<std::string>::iterator iter;
        char *token;
        const char *path = getenv("PATH");
        char *copy = (char *) malloc(strlen(path) + 1);
        strcpy(copy, path);
        token = strtok(copy, ":");

        while ( (token = strtok(0, ":")) != NULL) {
            pathV.push_back(std::string(token));
        }

        for (iter = pathV.begin(); iter < pathV.end(); iter++) {
            p = *iter + "/fts_url_copy";
            if (fexists(p.c_str()) == 0){
        	free(copy);
        	copy = NULL;
        	pathV.clear();
		return true;
	        }
        }

	free(copy);
	return false;
}

void myterminate(){
	FTS3_COMMON_LOGGER_NEWLOG(ERR) << "myterminate() was called" << commit;
}

void myunexpected(){
	FTS3_COMMON_LOGGER_NEWLOG(ERR) << "myunexpected() was called" << commit;
}


int main (int argc, char** argv)
{
    const char *hostcert = "/etc/grid-security/hostcert.pem";
    const char *configfile = "/etc/fts3/fts3config";

    try 
    {   
    	REGISTER_SIGNAL(SIGABRT);
    	REGISTER_SIGNAL(SIGSEGV);
    	REGISTER_SIGNAL(SIGTERM);
    	REGISTER_SIGNAL(SIGILL);
    	REGISTER_SIGNAL(SIGFPE);
    	REGISTER_SIGNAL(SIGBUS);
    	REGISTER_SIGNAL(SIGTRAP);
    	REGISTER_SIGNAL(SIGSYS);
	
        if (fexists(configfile) != 0){
		std::cerr << "fts3 server config file doesn't exist"  << std::endl;
		exit(1);	
	}	
	

        fts3::ws::GSoapDelegationHandler::init();
        StaticSslLocking::init_locks();
				
        FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv);
	std::string logDir = theServerConfig().get<std::string>("TransferLogDirectory");
	if(logDir.length() > 0){
		 logDir +="/fts3server.log";
		 int filedesc = open(logDir.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0777);
		 if(filedesc != -1){ //if ok
		 	close(filedesc);
		 	FILE* freopenLogFile = freopen (logDir.c_str(),"a",stderr);
			if(freopenLogFile == NULL){
		 		std::cerr << "fts3 server failed to open log file, errno is:" << strerror(errno) << std::endl;
				exit(1);			
			}
		 }else{
		 	std::cerr << "fts3 server failed to open log file, errno is:" << strerror(errno) << std::endl;
			exit(1);
		 }
	}

	if(false == checkUrlCopy()){
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Check if fts_url_copy process is set in the PATH env variable" << commit;
		exit(1);
	}
		
        if (fexists(hostcert) != 0){
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Check if hostcert/key are installed" << commit;
		exit(1);	
	}

   

        fts3_initialize_db_backend();
        struct sigaction action;
        action.sa_handler = _handle_sigint;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        int res = sigaction(SIGINT, &action, NULL);
	
	set_terminate (myterminate);
	set_unexpected (myunexpected);		
        
	std::string infosys = theServerConfig().get<std::string>("Infosys");
	if(infosys.length() > 0)
		setenv("LCG_GFAL_INFOSYS",infosys.c_str(),1);
            
        if (res == -1) 
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_System());
        }

        bool isDaemon = ! FTS3_CONFIG_NAMESPACE::theServerConfig().get<bool> ("no-daemon");

        if (isDaemon)
        {   
            daemonize();
            freopen (logDir.c_str(),"a",stderr);            
        }
        
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting server..." << commit;

        theServer().start();
	StaticSslLocking::kill_locks();
    }
    catch(Err& e)
    {
        std::string msg = "Fatal error, exiting...";
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
        return EXIT_FAILURE;
    }
    catch(...)
    {
        std::string msg = "Fatal error (unknown origin), exiting...";
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

