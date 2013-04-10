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
#include "ws/delegation/GSoapDelegationHandler.h"
#include <fstream>
#include "server.h"
#include "daemonize.h"
#include "signal_logger.h"
#include "StaticSslLocking.h"
#include <iomanip>
#include <sys/types.h>
#include <sys/wait.h>
#include "queue_updater.h"
#include "config/FileMonitor.h"
#include <boost/filesystem.hpp>
#include "name_to_uid.h"
#include <sys/resource.h>

namespace fs = boost::filesystem;

using namespace FTS3_SERVER_NAMESPACE;
using namespace FTS3_COMMON_NAMESPACE;

extern std::string stackTrace;
bool stopThreads = false;
const char *hostcert = "/etc/grid-security/hostcert.pem";
const char *configfile = "/etc/fts3/fts3config";

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
static void setLimits(){
	struct rlimit rl;
	int maxNumberOfProcesses = theServerConfig().get<int> ("MaxNumberOfProcesses");	
	if(maxNumberOfProcesses != -1){
		rl.rlim_cur = maxNumberOfProcesses;
		rl.rlim_max = maxNumberOfProcesses;
		if (setrlimit(RLIMIT_NPROC, &rl) == -1){
			FTS3_COMMON_LOGGER_NEWLOG(ERR) << "setrlimit nproc failed" << commit;
		}
	}				
}

static int fexists(const char *filename) {
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

void fts3_teardown_db_backend() {
    try {
        db::DBSingleton::tearDown();
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception when forcing the database teardown" << commit;
        exit(1);
    }
}

void _handle_sigint(int) {    
    if (stackTrace.length() > 0)
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << stackTrace << commit;
    stopThreads = true;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopping" << commit;
    sleep(4);
    theServer().stop();
    sleep(4);
    fts3_teardown_db_backend();
    StaticSslLocking::kill_locks();
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopped" << commit;
    exit(0);
}

/* -------------------------------------------------------------------------- */

void fts3_initialize_db_backend() {
    std::string dbUserName = theServerConfig().get<std::string > ("DbUserName");
    std::string dbPassword = theServerConfig().get<std::string > ("DbPassword");
    std::string dbConnectString = theServerConfig().get<std::string > ("DbConnectString");
    int pooledConn = theServerConfig().get<int> ("DbThreadsNum");

    try {
        db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString, pooledConn);
    } catch (Err& e) {
        throw;
    } catch (std::exception& ex) {
        throw;
    } catch (...) {
        throw;
    }


}

static bool checkUrlCopy() {
    std::string p("");
    std::vector<std::string> pathV;
    std::vector<std::string>::iterator iter;
    char *token=NULL;
    const char *path = getenv("PATH");
    char *copy = (char *) malloc(strlen(path) + 1);
    strcpy(copy, path);
    token = strtok(copy, ":");
    if(token)
    	pathV.push_back(std::string(token));
    while ((token = strtok(0, ":")) != NULL) {
        pathV.push_back(std::string(token));
    }

    for (iter = pathV.begin(); iter < pathV.end(); ++iter) {
        p = *iter + "/fts_url_copy";
        if (fexists(p.c_str()) == 0) {
            free(copy);
            copy = NULL;
            pathV.clear();
            return true;
        }
    }

    free(copy);
    return false;
}

void checkInitDirs(){
   
   std::vector<fs::path> paths;   
   vector<fs::path>::iterator iter;
    try {
   
   paths.push_back(fs::path("/etc/fts3"));
   paths.push_back(fs::path("/var/log/fts3"));   
   paths.push_back(fs::path("/var/lib/fts3"));      
   paths.push_back(fs::path("/var/lib/fts3/monitoring"));         
   paths.push_back(fs::path("/var/lib/fts3/status")); 
   paths.push_back(fs::path("/var/lib/fts3/stalled")); 
   paths.push_back(fs::path("/var/lib/fts3/logs")); 
   
   for (iter = paths.begin(); iter != paths.end(); ++iter) {
      if ( !fs::exists(*iter) || !fs::is_directory(*iter)){
	        if(fs::create_directory(*iter)){
			uid_t pw_uid;
                        pw_uid = name_to_uid();
			std::string path = (*iter).string();
                        int checkChown = chown(path.c_str(), pw_uid, getgid());
			if(checkChown!=0){
				FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to chmod for " << *iter << commit;
			}
		}else{
        		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Directory " << *iter << " does not exist or is not owned by fts3 user" << commit;
        		exit(1);          		
		}
      }          
   }      
    }catch (const fs::filesystem_error& ex){
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
        throw;    
    }catch (Err& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
        throw;
    } catch (std::exception& ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
        throw;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Something is going on with the filesystem required directories" << commit;
        throw;
    }
}


int DoServer(int argc, char** argv) {

    int res = 0;

    try {
        REGISTER_SIGNAL(SIGABRT);
        REGISTER_SIGNAL(SIGSEGV);
        REGISTER_SIGNAL(SIGTERM);
        REGISTER_SIGNAL(SIGILL);
        REGISTER_SIGNAL(SIGFPE);
        REGISTER_SIGNAL(SIGBUS);
        REGISTER_SIGNAL(SIGTRAP);
        REGISTER_SIGNAL(SIGSYS);
  	
	
	//set soft and hard limits
	setLimits();
	
        std::string arguments("");
        size_t foundHelp;
        if (argc > 1) {
            int i;
            for (i = 1; i < argc; i++) {
                arguments += argv[i];
            }
            foundHelp = arguments.find("-h");
            if (foundHelp != string::npos) {
                exit(0);
            }
        }

	//re-read here
        FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv, true);

        std::string logDir = theServerConfig().get<std::string > ("TransferLogDirectory");
        if (logDir.length() > 0) {
            logDir += "/fts3server.log";
            int filedesc = open(logDir.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (filedesc != -1) { //if ok
                close(filedesc);
                FILE* freopenLogFile = freopen(logDir.c_str(), "a", stderr);
                if (freopenLogFile == NULL) {
                    std::cerr << "fts3 server failed to open log file, errno is:" << strerror(errno) << std::endl;
                    exit(1);
                }
            } else {
                std::cerr << "fts3 server failed to open log file, errno is:" << strerror(errno) << std::endl;
                exit(1);
            }
        }

        bool isDaemon = !FTS3_CONFIG_NAMESPACE::theServerConfig().get<bool> ("no-daemon");

        if (isDaemon) {                
            FILE* openlog = freopen(logDir.c_str(), "a", stderr);
            if (openlog == NULL)
                std::cerr << "Can't open log file" << std::endl;
        }

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting server..." << commit;
        fts3::ws::GSoapDelegationHandler::init();
        StaticSslLocking::init_locks();
        fts3_initialize_db_backend();
        struct sigaction action;
        action.sa_handler = _handle_sigint;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        res = sigaction(SIGINT, &action, NULL);

        //initialize queue updater here to avoid race conditions  
        ThreadSafeList::get_instance();       
        theServer().start();
	
    } catch (Err& e) {        
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
        return -1;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Fatal error (unknown origin), exiting..." << commit;
        return -1;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {

    pid_t child;
    //very first check before it goes to deamon mode
    try{
    	if (fexists(configfile) != 0) {
            std::cerr << "fts3 server config file " << configfile << " doesn't exist" << std::endl;
            return EXIT_FAILURE;
        }
	
        if (false == checkUrlCopy()) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Check if fts_url_copy process is set in the PATH env variable" << commit;
            return EXIT_FAILURE;
        }

        if (fexists(hostcert) != 0) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Check if hostcert/key are installed" << commit;
            return EXIT_FAILURE;
        }	
	
    	FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv, true);
	
	checkInitDirs();	
    }catch (Err& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
        return EXIT_FAILURE;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Fatal error (unknown origin), exiting..." << commit;
        return EXIT_FAILURE;
    }

    std::string arguments("");
    size_t found;
    size_t foundHelp;
    int d = 0;
    if (argc > 1) {
        int i;
        for (i = 1; i < argc; i++) {
            arguments += argv[i];
        }
        found = arguments.find("-n");
        foundHelp = arguments.find("-h");
        if (found != string::npos) {
            {
                DoServer(argc, argv);
            }
            pthread_exit(0);
            return EXIT_SUCCESS;
        } else if (foundHelp != string::npos) {
            {
                DoServer(argc, argv);
            }
            pthread_exit(0);
            return EXIT_SUCCESS;
        }
        else {
            d = daemon(0, 0);
            if (d < 0)
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Can't set daemon, will continue attached to tty"  << commit;
        }
    } else {
        d = daemon(0, 0);
        if (d < 0)
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Can't set daemon, will continue attached to tty"  << commit;
    }

    int result = fork();

    if (result == 0) { //child
        int resultExec = DoServer(argc, argv);
	if (resultExec < 0) {
        	FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Can't start the server" << commit;
		exit(1);
    	}
    }else{ //parent
            child = result;	   
     	    sleep(2);
	    int err  = waitpid(child, NULL, WNOHANG);
	    if(err != 0){       
	        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "waitpid error: " << strerror(errno)  << commit; 
		return -1;
	    }
    }


    for (;;) {
        int status = 0;
        waitpid(-1, &status, 0);
        if (!WIFSTOPPED(status)) {
            result = fork();
            if (result == 0) {
                result = DoServer(argc, argv);
            }
            if (result < 0) {
                exit(1);
            }
        }
        sleep(15);
    }

    return 0;
}

