#include "SingleDbInstance.h"
#include <iostream>
#include <string>
#include <exception>
#include <vector>
#include "process.h"

using namespace db;

    void executeTransfer_a(std::string source, std::string dest) {
        const std::string cmd = "fts3_url_copy";
        std::string params = std::string("");
        ExecuteProcess *pr = NULL;

            params.append(" --source_url ");
            params.append(source);
            params.append(" --dest_url ");
            params.append(dest);
            pr = new ExecuteProcess(cmd, params, 1);
	    if(pr){
            	pr->executeProcessShell();
		delete pr;
	    }
    }


int main()
{
#ifdef FTS3_COMPILE_WITH_UNITTEST

try{
    //std::string DbUserName      = FTS3_CONFIG_NAMESPACE::theServerConfig().get<std::string>("DbUsername");  
    //std::cout << DbUserName << std::endl;
    //std::string DbPassword      = FTS3_CONFIG_NAMESPACE::theServerConfig().get<std::string>("DbPassword");      
    //std::cout << DbPassword << std::endl;    
    //std::string DbConnectString = FTS3_CONFIG_NAMESPACE::theServerConfig().get<std::string>("DbConnectString"); 	 

    const std::string temp = std::string("");
    const std::string requestID = "c8f3f3ad-2b34-11e1-9c6e-ca754d097ef5";
    const std::string dn ="/C=DE/O=GermanGrid/OU=DESY/CN=galway.desy.de";
    const std::string vo = std::string("dteam");
  /*
    std::map<std::string, std::string> src_dest_pair;
    src_dest_pair.insert(std::make_pair("SE1","SE2"));
    

    DBSingleton::instance().getDBObjectInstance()->submitPhysical(requestID, src_dest_pair, temp,
                                 dn, temp, vo, temp,
                                 temp, temp, temp, 
                                 temp, temp, temp, 1,
                                 temp, temp, temp);
	      
    JobStatus* record =  DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(requestID);
    
    if(record){
    	std::cout << "1  " << record->jobID << std::endl;
	std::cout << "2  "  << record->jobStatus << std::endl;
    	std::cout << "3  "  << record->channelName << std::endl;    
    	std::cout << "4  "  << record->clientDN << std::endl;    
    	std::cout << "5  "  << record->reason << std::endl;   
    	std::cout << "6  "  << record->voName << std::endl;   
    	std::cout << "7  "  << ctime((const time_t *) &(record->submitTime)) << std::endl;   
    	std::cout << "8  "  << record->numFiles << std::endl;    
    	std::cout << "9  "  << record->priority << std::endl;
	
	delete record;
	
    
    std::vector<JobStatus*> jobs;
    
    std::vector<std::string> inGivenStates;    
    
    std::string restrictToClientDN = std::string("");
    std::string forDN = std::string("");
    std::string VOname = std::string("");
    
    DBSingleton::instance().getDBObjectInstance()->init("msalicho", "Msal1973" , "oradev10.cern.ch:10520/D10");
    
    
    DBSingleton::instance().getDBObjectInstance()->listRequests(jobs, inGivenStates, restrictToClientDN, forDN, VOname );
    std::cout << jobs.size() << std::endl;
    DBSingleton::instance().getDBObjectInstance()->listRequests(jobs, inGivenStates, restrictToClientDN, forDN, vo );
    std::cout << jobs.size() << std::endl;
    DBSingleton::instance().getDBObjectInstance()->listRequests(jobs, inGivenStates, restrictToClientDN, dn, vo );    
    std::cout << jobs.size() << std::endl;
    DBSingleton::instance().getDBObjectInstance()->listRequests(jobs, inGivenStates, restrictToClientDN, dn, VOname );    
    std::cout << jobs.size() << std::endl;
    
    std::cout << "getSubmittedJobs" << std::endl;
    std::vector<TransferJobs*> jobs2;
    std::vector<TransferJobs*>::iterator iter2;    
    DBSingleton::instance().getDBObjectInstance()->getSubmittedJobs(jobs2);
    std::cout << jobs2.size() << std::endl;

        for (iter2 = jobs2.begin(); iter2 != jobs2.end(); ++iter2) {
	    TransferJobs* temp = (TransferJobs*) *iter2;
            std::cout << "JOB ID = " << temp->JOB_ID  << std::endl;
	    std::cout << "JOB STATE = " << temp->JOB_STATE  << std::endl;
        }    
	
	
   std::cout << "getByJobId" << std::endl;
   std::vector<TransferFiles*> files;
   std::vector<TransferFiles*>::iterator fileiter;    
   DBSingleton::instance().getDBObjectInstance()->getByJobId(jobs2,files);    
        for (fileiter = files.begin(); fileiter != files.end(); ++fileiter) {
	    TransferFiles* temp = (TransferFiles*) *fileiter;
            std::cout << "SOURCE URL= " << temp->SOURCE_SURL  << std::endl;
	    std::cout << "DEST URL = " << temp->DEST_SURL  << std::endl;
        }       
    */
    std::string source ="gsiftp://galway.desy.de:2811//pnfs/desy.de/data/dteam/file.outtest"; 
    std::string dest = "gsiftp://galway.desy.de:2811//pnfs/desy.de/data/dteam/file.outtest134453b";
    executeTransfer_a(source, dest);
    
  }
catch (std::string const &e)
  {
    std::cout << e<< std::endl;
  }
  
#endif      
    return (0);
}
