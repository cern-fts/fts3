#include "SingleDbInstance.h"
#include <iostream>
#include <string>
#include <exception>
#include <vector>
#include "ReadConfigFile.h"

using namespace db;

int main()
{
try{
    std::string requestID = "c8f3f3ad-2b34-11e1-9c6e-ca754d097ef6";    

    DBSingleton::instance().getDBObjectInstance()->init(ReadConfigFile::instance().getDBUsername(), ReadConfigFile::instance().getDBPassword(), ReadConfigFile::instance().getDBConnectionString());
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
    
    DBSingleton::instance().getDBObjectInstance()->removeGroupMember("groupName", "siteName");
    
    DBSingleton::instance().getDBObjectInstance()->addGroupMember("groupName", "siteName");
    DBSingleton::instance().getDBObjectInstance()->addGroupMember("groupName", "siteName");
    
    std::vector<std::string> test = DBSingleton::instance().getDBObjectInstance()->getSiteGroupMembers("groupName");
    std::cout << test.size() << std::endl;
    
    
    std::vector<std::string> test2 = DBSingleton::instance().getDBObjectInstance()->getSiteGroupMembers("groupName");
    std::cout << test2.size() << std::endl;
   
  }
catch (std::string const &e)
  {
    std::cout << e<< std::endl;
  }
  
      
    return (0);
}
