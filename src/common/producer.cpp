#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "producer_consumer_common.h"
#include "uuid_generator.h"
using namespace std;



void getUniqueTempFileName(const std::string& basename,
                                           std::string& tempname)
{
    int iRes;
    do 
    {
        std::string uuidGen = UuidGenerator::generateUUID();
        time_t tmCurrent = time(NULL);
        std::stringstream strmName;
        strmName << basename << uuidGen << "_" << tmCurrent;
        tempname = strmName.str();

        struct stat st;
        iRes = stat(tempname.c_str(),
                    &st);
    }
    while( 0 == iRes );
}

void mktempfile(const std::string& basename, 
            std::string& tempname)
{
   char* temp= (char *) "_XXXXXX";
   mktemp(temp);

   tempname = basename;
   tempname.append(std::string(temp));

}

void runProducerMonitoring(const char* msg)
{
		FILE *fp=NULL; 
		std::string basename("/var/lib/fts3/monitoring/");
	        std::string tempname;

	        getUniqueTempFileName(basename, tempname);
       		if ((fp = fopen(tempname.c_str(), "w")) != NULL)
       		{        					
        		fwrite(msg, strlen(msg)+1, 1, fp); 
        		fclose(fp);
			std::string renamedFile = tempname + "_ready";
			rename(tempname.c_str(), renamedFile.c_str());
			unlink(tempname.c_str());
	       }		
}


void runProducerStatus(struct message msg){
		FILE *fp=NULL; 
		std::string basename("/var/lib/fts3/status/");
	        std::string tempname;

	        getUniqueTempFileName(basename, tempname);
       		if ((fp = fopen(tempname.c_str(), "w")) != NULL)
       		{        					
        		fwrite(&msg, sizeof(msg), 1, fp); 
        		fclose(fp);
			std::string renamedFile = tempname + "_ready";
			rename(tempname.c_str(), renamedFile.c_str());
			unlink(tempname.c_str());
	       }		
}


void runProducerStall(struct message_updater msg){
		FILE *fp=NULL; 
		std::string basename("/var/lib/fts3/stalled/");
	        std::string tempname = basename + "_" + msg.job_id + "_" + boost::lexical_cast<string>( msg.file_id );
       		if ((fp = fopen(tempname.c_str(), "w+")) != NULL)
       		{        					
        		fwrite(&msg, sizeof(msg), 1, fp); 
        		fclose(fp);
			std::string renamedFile = tempname + "_ready";
			rename(tempname.c_str(), renamedFile.c_str());
			unlink(tempname.c_str());
	       }		
}

