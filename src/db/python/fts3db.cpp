/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

#include <boost/python.hpp>
#include <common/error.h>
#include <config/serverconfig.h>
#include <object.h>
#include <pyerrors.h>
#include "MonitoringDbWrapper.h"

using namespace boost::python;

// Prototype
void export_types(void);

// Simple config wrap
std::string getConfig(const std::string& key)
{
  return fts3::config::theServerConfig().get<std::string>(key);
}

// Exception translation
PyObject* errException;

void errTranslator(const fts3::common::Err& e) {
    object pythonExceptionInstance(e);
    PyErr_SetObject(errException, pythonExceptionInstance.ptr());
}

// Entry point
BOOST_PYTHON_MODULE(ftsdb)
{
  // Config helper
  def("getConfig", getConfig);
  
  // Exception
  //class_<fts3::common::Err> exceptionClass("FTSError", no_init);
  //exceptionClass.def("what", &fts3::common::Err::what);

  //errException = exceptionClass.ptr();
  //register_exception_translator<fts3::common::Err>(&errTranslator);
  
  // Types
  export_types();

  // Monitoring DB interface
  unsigned (MonitoringDbWrapper::*ntransfers)(const std::string&, const boost::python::list&);
  unsigned (MonitoringDbWrapper::*ntransfersPair)(const std::string&, const SourceAndDestSE&, const boost::python::list&);

  ntransfers = &MonitoringDbWrapper::numberOfTransfersInState;
  ntransfersPair = &MonitoringDbWrapper::numberOfTransfersInState;

  class_<MonitoringDbWrapper, boost::noncopyable>("MonitoringDb", no_init)
      .def("getInstance", &MonitoringDbWrapper::getInstance, return_value_policy<reference_existing_object>())
      .staticmethod("getInstance")
      .def("init", &MonitoringDbWrapper::init)
      .def("setNotBefore", &MonitoringDbWrapper::setNotBefore)
      .def("getVONames", &MonitoringDbWrapper::getVONames)
      .def("getSourceAndDestSEForVO", &MonitoringDbWrapper::getSourceAndDestSEForVO)
      .def("numberOfJobsWithState", &MonitoringDbWrapper::numberOfJobsInState)
      .def("getConfigAudit", &MonitoringDbWrapper::getConfigAudit)
      .def("getTransferFiles", &MonitoringDbWrapper::getTransferFiles)
      .def("getJob", &MonitoringDbWrapper::getJob)
      .def("filterJobs", &MonitoringDbWrapper::filterJobs)
      .def("numberOfTransfersInState", ntransfers)
      .def("numberOfTransfersInState", ntransfersPair)
      .def("getUniqueReasons", &MonitoringDbWrapper::getUniqueReasons)
      .def("averageDurationPerSePair", &MonitoringDbWrapper::averageDurationPerSePair)
      .def("averageThroughputPerSePair", &MonitoringDbWrapper::averageThroughputPerSePair)
      .def("getJobVOAndSites", &MonitoringDbWrapper::getJobVOAndSites);
}
