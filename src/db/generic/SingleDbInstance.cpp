#include "SingleDbInstance.h"
#include <fstream>
#include "logger.h"
#include "error.h"
#include "config/serverconfig.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

using namespace FTS3_COMMON_NAMESPACE;
using namespace FTS3_CONFIG_NAMESPACE;

namespace db{

boost::scoped_ptr<DBSingleton> DBSingleton::i;

// Implementation

DBSingleton::DBSingleton() {

  try{
  std::string dbType = theServerConfig().get<std::string>("DbType");
    
    if (dbType != "oracle")
    {   
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(dbType + " backend is not supported."));
    }

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << dbType << " database backend loaded" << commit; 
    
    //hardcoded for now
    libraryFileName = "libfts3_db_oracle.so";

    dlm = new DynamicLibraryManager(libraryFileName);
    if (dlm && dlm->isLibraryLoaded()) {

        DynamicLibraryManager::Symbol symbolInstatiate = dlm->findSymbol("create");

        DynamicLibraryManager::Symbol symbolDestroy = dlm->findSymbol("destroy");

        *(void**)( &create_db ) =  symbolInstatiate;

        *(void**)( &destroy_db ) = symbolDestroy;

        // create an instance of the DB class
        dbBackend = create_db();
    } else{
    	FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Failed to load database plugin library"));
    }
    
    }catch(...) {
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Failed to load database plugin library"));       
    }

}

DBSingleton::~DBSingleton() {
    if (dbBackend)
        destroy_db(dbBackend);
    if (dlm)
        delete dlm;
}
}






/*
#ifdef FTS3_COMPILE_WITH_UNITTEST
BOOST_AUTO_TEST_SUITE(db_test_suite)

BOOST_AUTO_TEST_CASE (DB_test)
{
    using namespace db;

    const std::string temp = std::string("");
    const std::string requestID = "c8f3f3ad-2b34-11e1-9c6e-ca754d097ef5";
    const std::string dn ="/C=DE/O=GermanGrid/OU=DESY/CN=galway.desy.de";
    const std::string vo = std::string("dteam");
    std::map<std::string, std::string> src_dest_pair;
    src_dest_pair.insert(std::make_pair("SE1","SE2"));
    
    try{    	
    	DBSingleton::instance().getDBObjectInstance()->init("msalicho", "Msal1973" , "oradev10.cern.ch:10520/D10");
    }
    catch(const std::exception &e){
    	BOOST_WARN( !e.what() );
	
    }
    
        try{
    DBSingleton::instance().getDBObjectInstance()->submitPhysical(requestID, src_dest_pair, temp,
                                 dn, temp, vo, temp,
                                 temp, temp, temp, 
                                 temp, temp, temp, 1,
                                 temp, temp, temp);
    }
    catch(const std::exception &e){
    	BOOST_WARN( !e.what() );
	
    }
    
    
    try{	     
    JobStatus* record =  DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(requestID);
    BOOST_CHECK( !record );
    delete record;
    }
    catch(const std::exception &e){
    	BOOST_WARN( !e.what() );
	
    }
    
}

BOOST_AUTO_TEST_SUITE_END()
#endif      
*/
