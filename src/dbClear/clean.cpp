#include<iostream>
#include <cstdio>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "common/error.h"
#include "common/logger.h"
#include "config/serverconfig.h"
#include "server.h"
#include "db/generic/SingleDbInstance.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace FTS3_SERVER_NAMESPACE;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;

/* -------------------------------------------------------------------------- */
void fts3_initialize_db_backend()
{
    std::string dbUserName = theServerConfig().get<std::string>("DbUserName");
    std::string dbPassword = theServerConfig().get<std::string>("DbPassword");
    std::string dbConnectString = theServerConfig().get<std::string>("DbConnectString");
    
    try{
    	db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString, 1);
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

static int fexists(const char *filename) {
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}



int main(int argc, char** argv){

int d = daemon(0,0);
if(d < 0)
	std::cerr << "Can't set daemon, will continue attached to tty" << std::endl;  

try 
    { 
	const char *configfile = "/etc/fts3/fts3config";	

        if (fexists(configfile) != 0){
		std::cerr << "fts3 server config file doesn't exist"  << std::endl;
		exit(1);	
	}	
	
        FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv);
	fts3_initialize_db_backend();
	
	std::string cleanRecordsHost = theServerConfig().get<std::string>("CleanRecordsHost");
	if(cleanRecordsHost.compare("true")==0)
		db::DBSingleton::instance().getDBObjectInstance()->backup();
      }
    catch(...){
    	return EXIT_FAILURE;
    }    
      
  return EXIT_SUCCESS;
}
