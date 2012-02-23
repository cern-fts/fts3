#include "SingleDbInstance.h"
#include <iostream>
#include <string>
#include <exception>
#include <vector>
#include "serverconfig.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS



using namespace db;

int main()
{
#ifdef FTS3_COMPILE_WITH_UNITTEST

try{
    //std::string DbUserName      = FTS3_CONFIG_NAMESPACE::theServerConfig().get<std::string>("DbUsername");  
    //std::cout << DbUserName << std::endl;
    //std::string DbPassword      = FTS3_CONFIG_NAMESPACE::theServerConfig().get<std::string>("DbPassword");      
    //std::cout << DbPassword << std::endl;    
    //std::string DbConnectString = FTS3_CONFIG_NAMESPACE::theServerConfig().get<std::string>("DbConnectString"); 	 

    DBSingleton::instance().getDBObjectInstance()->init("msalicho", "Msal1973" , "oradev10.cern.ch:10520/D10");

    const std::string temp = std::string("");
    const std::string requestID = "c8f3f3ad-2b34-11e1-9c6e-ca754d097ef5";
    const std::string dn ="/C=DE/O=GermanGrid/OU=DESY/CN=galway.desy.de";
    const std::string vo = std::string("dteam");

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
    }
  }
catch (std::string const &e)
  {
    std::cout << e<< std::endl;
  }
  
#endif      
    return (0);
}
