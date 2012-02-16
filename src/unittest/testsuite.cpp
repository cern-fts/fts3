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

/** \file testsuite.cpp FTS3 test suite runner. */

#include "common/dev.h"
#include "cppunit_wrapper.h"

int main ()
{
    CPPUNIT_NS::TestResult testresult;
    CPPUNIT_NS::TestResultCollector collectedresults;
    testresult.addListener (&collectedresults);
    CPPUNIT_NS::BriefTestProgressListener progress;
    testresult.addListener (&progress);
    CPPUNIT_NS::TestRunner testrunner;
    
    testrunner.addTest
    (
        CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest ()
    );
    
    testrunner.run(testresult);
    CPPUNIT_NS::CompilerOutputter compileroutputter (&collectedresults, std::cerr);
    compileroutputter.write();

    return collectedresults.wasSuccessful() ? 0 : 1;
}
