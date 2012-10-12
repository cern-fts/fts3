#include <boost/python.hpp>
#include <config/serverconfig.h>
#include <db/generic/JobStatus.h>
#include <db/generic/TransferJobs.h>
#include "DbIfceWrapper.h"

using namespace boost::python;

// Prototypes
extern void export_job_types(void);
extern void export_se_types(void);

// Simple config wrap
std::string getConfig(const std::string& key)
{
  return fts3::config::theServerConfig().get<std::string>(key);
}

// Entry point
BOOST_PYTHON_MODULE(fts3db)
{
  // Config helper
  def("getConfig", getConfig);
  
  // Types
  export_job_types();
  export_se_types();
  
  // Wrapper
  class_<DbIfceWrapper, boost::noncopyable>("DbIfce", no_init)
    .def("getInstance", &DbIfceWrapper::getInstance, return_value_policy<reference_existing_object>())
    .staticmethod("getInstance")
    .def("init", &DbIfceWrapper::init)
    .def("getByJobId", &DbIfceWrapper::getByJobId)
    .def("getTransferFileStatus", &DbIfceWrapper::getTransferFileStatus)
    .def("getTransferJobStatus", &DbIfceWrapper::getTransferJobStatus)
    .def("getSubmittedJobs", &DbIfceWrapper::getSubmittedJobs)
    .def("getSiteGroupNames", &DbIfceWrapper::getSiteGroupNames)
    .def("getSiteGroupMembers", &DbIfceWrapper::getSiteGroupMembers)
    .def("listRequests", &DbIfceWrapper::listRequests)
    .def("getSe", &DbIfceWrapper::getSe)
    .def("getAllSeInfoNoCriteria", &DbIfceWrapper::getAllSeInfoNoCriteria)
    .def("getAllMatchingSeNames", &DbIfceWrapper::getAllMatchingSeNames)
    .def("getAllMatchingSeGroupNames", &DbIfceWrapper::getAllMatchingSeGroupNames)
    .def("getAllShareConfigNoCriteria", &DbIfceWrapper::getAllShareConfigNoCriteria)
    .def("getAllShareAndConfigWithCriteria", &DbIfceWrapper::getAllShareAndConfigWithCriteria)
    .def("getSeCreditsInUse", &DbIfceWrapper::getSeCreditsInUse)
    .def("getGroupCreditsInUse", &DbIfceWrapper::getGroupCreditsInUse);
}
