#include "SingleDbInstance.h"
#include <fstream>
#include "Logger.h"
#include "ReadConfigFile.h"

namespace db{

boost::scoped_ptr<DBSingleton> DBSingleton::i;
Mutex DBSingleton::m;

// Implementation

DBSingleton::DBSingleton() {

    if (ReadConfigFile::instance().isDBCfgValid()) {
        libraryFileName = ReadConfigFile::instance().getDBLibName();
    } else {
        Logger::instance().error("Cannot load DB library, check config file");
        exit(1);
    }

    dlm = new DynamicLibraryManager(libraryFileName);
    if (dlm) {

        DynamicLibraryManager::Symbol symbolInstatiate = dlm->findSymbol("create");

        DynamicLibraryManager::Symbol symbolDestroy = dlm->findSymbol("destroy");

        create_db = (create_t*) symbolInstatiate;

        destroy_db = (destroy_t*) symbolDestroy;

        // create an instance of the DB class
        dbBackend = create_db();
    } else {
        Logger::instance().error("Dynamic library cannot be loaded, check configuration in /etc/fts3config");
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
