IF (GLOBUS_LOCATION)
    FIND_PATH(GLOBUS_INCLUDE_DIRS globus_gsi_credential.h
              HINTS ${GLOBUS_LOCATION}/include/*)
    FIND_LIBRARY (GLOBUS_LIBRARIES
                  NAMES globus_gsi_credential globus_gsi_credential_gcc64
                  HINTS ${GLOBUS_LOCATION}/lib/ ${GLOBUS_LOCATION}/lib64/)
              
    MESSAGE ("Found Globus includes: ${GLOBUS_INCLUDE_DIRS}")
    MESSAGE ("Found Globus library: ${GLOBUS_LIBRARIES}")
              
    MARK_AS_ADVANCED (GLOBUS_INCLUDE_DIRS GLOBUS_LIBRARIES)
ELSE (GLOBUS_LOCATION)
    FIND_PACKAGE(PkgConfig)
    
    PKG_SEARCH_MODULE(GLOBUS "REQUIRED" globus-gsi-credential)
    
    IF(GLOBUS_FOUND)
      MESSAGE(STATUS "Found Globus")
    ENDIF(GLOBUS_FOUND)
ENDIF (GLOBUS_LOCATION)
