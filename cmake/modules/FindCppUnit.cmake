# Copyright @ Members of the EMI Collaboration, 2010.
# See www.eu-emi.eu for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. 
# You may obtain a copy of the License at 
#
#    http://www.apache.org/licenses/LICENSE-2.0 
#
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# See the License for the specific language governing permissions and 
# limitations under the License. 

# - Find Oracle CppUnit.
# Find CppUnit includes and libraries.
#
# It defines the following variables:
# CPPUNIT_LIBRARY - the library
# CPPUNIT_INCLUDE_DIR - the headers
# CPPUNIT_FOUND - Set to false, or undefined, if we haven't found

#include(LibFindMacros)

# Finally the library itself
find_library(CPPUNIT_LIBRARY
  NAMES cppunit
  
  PATHS
	ENV LD_LIBRARY_PATH
	"~/usr/lib"
	"/usr/local/lib"
	"/usr/lib64"
)

# locate header files
find_path(CPPUNIT_INCLUDE_DIR
  NAMES TestSuite.h
  PATHS
	"~/usr/include/cppunit"
	"/usr/local/include/cppunit"
	"/usr/local/cppunit"
	"/usr/include/cppunit"
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(CPPUNIT_PROCESS_INCLUDES CPPUNIT_INCLUDE_DIR)
set(CPPUNIT_PROCESS_LIBS CPPUNIT_LIBRARY)

MESSAGE( STATUS "CPPUNIT_LIBRARY: " ${CPPUNIT_LIBRARY} )
MESSAGE( STATUS "CPPUNIT_INCLUDE_DIR: " ${CPPUNIT_INCLUDE_DIR} )
