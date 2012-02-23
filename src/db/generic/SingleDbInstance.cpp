#include "SingleDbInstance.h"
#include <fstream>
#include "logger.h"



namespace db{

boost::scoped_ptr<DBSingleton> DBSingleton::i;

// Implementation

DBSingleton::DBSingleton() {

  try{
    //hardcoded for now
    libraryFileName = "/home/user/workspace/fts3svn/build/src/db/oracle/libfts3_db_oracle.so";

    dlm = new DynamicLibraryManager(libraryFileName);
    if (dlm) {

        DynamicLibraryManager::Symbol symbolInstatiate = dlm->findSymbol("create");

        DynamicLibraryManager::Symbol symbolDestroy = dlm->findSymbol("destroy");

        *(void**)( &create_db ) =  symbolInstatiate;

        *(void**)( &destroy_db ) = symbolDestroy;

        // create an instance of the DB class
        dbBackend = create_db();
    } 
    
    }catch(...) {
        FTS3_COMMON_LOGGER_LOG(ERR,"Dynamic database library cannot be loaded, check configuration file" );       
    }

}

DBSingleton::~DBSingleton() {
    if (dbBackend)
        destroy_db(dbBackend);
    if (dlm)
        delete dlm;
}
}
