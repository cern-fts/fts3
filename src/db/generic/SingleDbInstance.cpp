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
    	FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Failed to load database plugin library, check if libfts3_db_oracle.so path is set in LD_LIBRARY_PATH or /usr/lib64"));
	exit(1);
    }
    
    }catch(...) {
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Failed to load database plugin library, check if libfts3_db_oracle.so path is set in LD_LIBRARY_PATH or /usr/lib64"));       
	exit(1);
    }

}

DBSingleton::~DBSingleton() {
    if (dbBackend)
        destroy_db(dbBackend);
    if (dlm)
        delete dlm;
}
}






