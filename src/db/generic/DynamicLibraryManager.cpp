#include "DynamicLibraryManager.h"
#include <dlfcn.h>
#include <unistd.h>
#include <iostream>
#include <memory>

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
        std::auto_ptr<DynamicLibraryManagerException> f(new DynamicLibraryManagerException(m_libraryName,
                symbol,
                DynamicLibraryManagerException::symbolNotFound));
        throw f;
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
        std::auto_ptr<DynamicLibraryManagerException> f(new DynamicLibraryManagerException(m_libraryName, "",
                DynamicLibraryManagerException::loadingFailed));
        throw f;
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










