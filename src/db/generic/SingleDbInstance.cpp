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
ThreadTraits::MUTEX DBSingleton::_mutex;

// Implementation

DBSingleton::DBSingleton(): dbBackend(NULL), monitoringDbBackend(NULL) {

  try{

  std::string dbType = theServerConfig().get<std::string>("DbType");
    
    if (dbType != "oracle")
    {   
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(dbType + " backend is not supported."));
    }

    //hardcoded for now
    libraryFileName = "libfts_db_oracle.so";

    dlm = new DynamicLibraryManager(libraryFileName);
    if (dlm && dlm->isLibraryLoaded()) {

        DynamicLibraryManager::Symbol symbolInstatiate = dlm->findSymbol("create");

        DynamicLibraryManager::Symbol symbolDestroy = dlm->findSymbol("destroy");

        *(void**)( &create_db ) =  symbolInstatiate;

        *(void**)( &destroy_db ) = symbolDestroy;

        DynamicLibraryManager::Symbol symbolInstatiateMonitoring = dlm->findSymbol("create_monitoring");

        DynamicLibraryManager::Symbol symbolDestroyMonitoring = dlm->findSymbol("destroy_monitoring");

        *(void**)( &create_monitoring_db ) =  symbolInstatiateMonitoring;

        *(void**)( &destroy_monitoring_db ) = symbolDestroyMonitoring;

        // create an instance of the DB class
        dbBackend = create_db();

        // create monitoring db on request
    } else{
    	FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Failed to load database plugin library, check if libfts_db_oracle.so path is set in LD_LIBRARY_PATH or /usr/lib64"));
	throw Err_Custom("Failed to load database plugin library, check if libfts_db_oracle.so path is set in LD_LIBRARY_PATH or /usr/lib64");
    }
    
    }catch(...) {
        //FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Failed to load database plugin library, check if libfts_db_oracle.so path is set in LD_LIBRARY_PATH or /usr/lib64"));
	throw Err_Custom("Failed to load database plugin library, check if libfts_db_oracle.so path is set in LD_LIBRARY_PATH or /usr/lib64");
    }

}

DBSingleton::~DBSingleton() {
    if (dbBackend)
        destroy_db(dbBackend);
    if (monitoringDbBackend)
        destroy_monitoring_db(monitoringDbBackend);
    if (dlm)
        delete dlm;
}
}






