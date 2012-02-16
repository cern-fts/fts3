/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License. 
You may obtain a copy of the License at 

    http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and 
limitations under the License. */

/** \file cppunit_wrapper.h Isolate CppUnit header file structure from the
  rest of the code. */

#ifndef FTS3_UNITTEST_CPPUNIT_WRAPPER_H
#define FTS3_UNITTEST_CPPUNIT_WRAPPER_H

// Disable warnings triggered by the CppUnit headers
// See http://wiki.services.openoffice.org/wiki/Writing_warning-free_code
// See also http://www.artima.com/cppsource/codestandards.html
#if defined __GNUC__
#  pragma GCC system_header
#elif defined __SUNPRO_CC
#  pragma disable_warn
#elif defined _MSC_VER
#  pragma warning(push, 1)
#endif

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>

#if defined __SUNPRO_CC
#  pragma enable_warn
#elif defined _MSC_VER
#  pragma warning(pop)
#endif

#endif /* FTS3_UNITTEST_CPPUNIT_WRAPPER_H */

