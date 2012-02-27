#include "DynamicLibraryManager.h"
#include "error.h"
#include <dlfcn.h>
#include <unistd.h>
#include <iostream>
#include <memory>


using namespace FTS3_COMMON_NAMESPACE;

DynamicLibraryManager::DynamicLibraryManager(const std::string &libraryFileName)
: m_libraryHandle(NULL)
, m_libraryName(libraryFileName) {
    loadLibrary(libraryFileName);
}

DynamicLibraryManager::~DynamicLibraryManager() {
    releaseLibrary();
}

DynamicLibraryManager::Symbol
DynamicLibraryManager::findSymbol(const std::string &symbol) {
    try {
        Symbol symbolPointer = doFindSymbol(symbol);
        if (symbolPointer != NULL)
            return symbolPointer;
    } catch (...) {		
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Symbol cannot be found in the database plugin library"));
    }

    return NULL; // keep compiler happy
}

void
DynamicLibraryManager::loadLibrary(const std::string &libraryName) {
    try {
        releaseLibrary();
        m_libraryHandle = doLoadLibrary(libraryName);
        if (m_libraryHandle != NULL)
            return;
    } catch (...) {
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Failed to load database plugin library"));
    }
}

void
DynamicLibraryManager::releaseLibrary() {
    if (m_libraryHandle != NULL) {
        doReleaseLibrary();
        m_libraryHandle = NULL;
    }
}

DynamicLibraryManager::LibraryHandle
DynamicLibraryManager::doLoadLibrary(const std::string &libraryName) {
    return ::dlopen(libraryName.c_str(), RTLD_LAZY);
}

void DynamicLibraryManager::doReleaseLibrary() {
    ::dlclose(m_libraryHandle);
}

DynamicLibraryManager::Symbol DynamicLibraryManager::doFindSymbol(const std::string &symbol) {
    return ::dlsym(m_libraryHandle, symbol.c_str());
}


