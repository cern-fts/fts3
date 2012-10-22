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
  class_<fts3::common::Err> exceptionClass("FTSError", no_init);
  exceptionClass.def("what", &fts3::common::Err::what);

  errException = exceptionClass.ptr();
  register_exception_translator<fts3::common::Err>(&errTranslator);
  
  // Types
  export_types();

  // Monitoring DB interface
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
      .def("numberOfTransfersInState", &MonitoringDbWrapper::numberOfTransfersInState)
      .def("getUniqueReasons", &MonitoringDbWrapper::getUniqueReasons)
      .def("averageDurationPerSePair", &MonitoringDbWrapper::averageDurationPerSePair)
      .def("averageThroughputPerSePair", &MonitoringDbWrapper::averageThroughputPerSePair)
      .def("getJobVOAndSites", &MonitoringDbWrapper::getJobVOAndSites);
}
