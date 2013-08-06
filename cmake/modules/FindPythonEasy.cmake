# - Find python libraries
#  This module find the current version of python on your installation in a easy way
#
#  PYTHON_LIBRARIES     			= path to the python library
#  PYTHON_LIBRARIES_${VERSION}    	= path to the python library for the distribution version
#  PYTHON_SITE_PACKAGES_${_VERSION} = path to the python modules dir
#  PYTHON_LIBRARIES   				= path to the python modules dir for the distribution version
#  PYTHON_INCLUDE_PATH  			= path to where Python.h is found
#  PYTHON_INCLUDE_PATH_${VERSION}  	= path to where Python.h for the distribution version
#  PYTHON_EXECUTABLE 				=  python interpreter for the distribution version
#  PYTHON_EXECUTABLE_${VERSION} 	=  available python version
#  PYTHON_AVAILABLE_VERSIONS		= list all the version available on the system
# --

LIST(APPEND L_PYTHON_VERSIONS "1.5" "1.6" "2.0" "2.1" "2.2" "2.4" "2.5" "2.6" "2.7" "2.8" "3.0" "3.1" "3.2" "3.3" "3.4")

INCLUDE(FindPackageHandleStandardArgs)

 # determine the std version
 
 # main version executable
 FIND_PROGRAM(PYTHON_EXECUTABLE
  NAMES python
  PATHS
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.4\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.3\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.2\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.1\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.0\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.8\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.6\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.5\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.4\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.3\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.2\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.1\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.0\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.6\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.5\\InstallPath]
 )


EXECUTE_PROCESS( COMMAND ${PYTHON_EXECUTABLE} -c "import sys; print('%s.%s' % sys.version_info[:2])" 
					OUTPUT_VARIABLE PYTHON_CURRENT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
					



## tests for all versions of the  packages

FOREACH(_VERSION ${L_PYTHON_VERSIONS})

  STRING(REPLACE "." "" _VERSION_NO_DOTS ${_VERSION})

FIND_PROGRAM(PYTHON_EXECUTABLE_${_VERSION}
  NAMES python${_VERSION}
  PATHS
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.4\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.3\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.2\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.1\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.0\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.8\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.6\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.5\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.4\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.3\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.2\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.1\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.0\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.6\\InstallPath]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.5\\InstallPath]
 )
 
 
 IF(PYTHON_EXECUTABLE_${_VERSION})
 
		LIST(APPEND PYTHON_AVAILABLE_VERSIONS ${_VERSION})
 
                EXECUTE_PROCESS( COMMAND ${PYTHON_EXECUTABLE_${_VERSION}} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(True))"
						OUTPUT_VARIABLE PYTHON_SITE_PACKAGES_${_VERSION} OUTPUT_STRIP_TRAILING_WHITESPACE)
 
		#find libs
		FIND_LIBRARY(PYTHON_LIBRARY_${_VERSION}
			NAMES python${_VERSION_NO_DOTS} python${_VERSION}
			PATHS
			[HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_VERSION}\\InstallPath]/libs
			PATH_SUFFIXES
			# Avoid finding the .dll in the PATH.  We want the .lib.
			NO_SYSTEM_ENVIRONMENT_PATH
	 	)
		SET(PYTHON_LIBRARIES_${_VERSION} ${PYTHON_LIBRARY_${_VERSION}})

	 # find path
	  FIND_PATH(PYTHON_INCLUDE_PATH_${_VERSION}
		NAMES Python.h
		PATHS
		  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_VERSION}\\InstallPath]/include
		PATH_SUFFIXES
		  python${_VERSION}
	  )
  
	
	
		FIND_PACKAGE_HANDLE_STANDARD_ARGS(Python${_VERSION} DEFAULT_MSG PYTHON_EXECUTABLE_${_VERSION})

		MARK_AS_ADVANCED(PYTHON_EXECUTABLE_${_VERSION})
		MARK_AS_ADVANCED(PYTHON_SITE_PACKAGES_${_VERSION})	
		MARK_AS_ADVANCED(PYTHON_LIBRARIES_${_VERSION})	
		MARK_AS_ADVANCED(PYTHON_INCLUDE_PATH_${_VERSION})	
 ENDIF(PYTHON_EXECUTABLE_${_VERSION})
 
 ENDFOREACH(_VERSION ${L_PYTHON_VERSIONS})

 
SET(PYTHON_SITE_PACKAGES ${PYTHON_SITE_PACKAGES_${PYTHON_CURRENT_VERSION}}
	CACHE PATH "path to the python modules dir for the distribution version")
	
SET(PYTHON_LIBRARIES ${PYTHON_LIBRARIES_${PYTHON_CURRENT_VERSION}}
	CACHE PATH "path to the python modules dir for the distribution version")	
	
SET(PYTHON_INCLUDE_PATH ${PYTHON_INCLUDE_PATH_${PYTHON_CURRENT_VERSION}}
	CACHE PATH "path to the python include dir for the distribution version")	

FIND_PACKAGE_HANDLE_STANDARD_ARGS(PythonCurrentVersion DEFAULT_MSG PYTHON_CURRENT_VERSION )
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PythonCurrentInclude DEFAULT_MSG PYTHON_INCLUDE_PATH )
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PythonCurrentLibs DEFAULT_MSG PYTHON_LIBRARIES )
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PythonCurrentModsDir DEFAULT_MSG PYTHON_SITE_PACKAGES )

 

MARK_AS_ADVANCED(PYTHON_EXECUTABLE) 
MARK_AS_ADVANCED(PYTHON_SITE_PACKAGES)	
MARK_AS_ADVANCED(PYTHON_LIBRARIES)	
MARK_AS_ADVANCED(PYTHON_INCLUDE_PATH)	

