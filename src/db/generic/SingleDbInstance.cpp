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

    std::string dbType = theServerConfig().get<std::string>("DbType");

    libraryFileName = "libfts_db_";
    libraryFileName += dbType;
    libraryFileName += ".so";

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
    } else {
	throw Err_Custom(dlm->getLastError());
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






