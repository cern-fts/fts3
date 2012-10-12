#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <db/generic/JobStatus.h>
#include <db/generic/TransferJobs.h>
#include <db/generic/FileTransferStatus.h>

using namespace boost::python;



inline bool operator == (const TransferJobs& a, const TransferJobs& b)
{
  return a.JOB_ID == b.JOB_ID;
}



inline bool operator == (const JobStatus& a, const JobStatus& b)
{
  return a.jobID == b.jobID && a.jobStatus == b.jobStatus;
}



inline bool operator == (const FileTransferStatus& a, const FileTransferStatus& b)
{
  return a.sourceSURL == b.sourceSURL && a.destSURL == b.destSURL && a.start_time == b.start_time;
}



void export_job_types(void)
{
  // Job status and vector
  class_<JobStatus>("JobStatus", init<>())
    .def_readonly("jobID", &JobStatus::jobID)
    .def_readonly("jobStatus", &JobStatus::jobStatus)
    .def_readonly("fileStatus", &JobStatus::fileStatus)
    .def_readonly("clientDN", &JobStatus::clientDN)
    .def_readonly("reason", &JobStatus::reason)
    .def_readonly("voName", &JobStatus::voName)
    .def_readonly("submitTime", &JobStatus::submitTime)
    .def_readonly("numFiles", &JobStatus::numFiles)
    .def_readonly("priority", &JobStatus::priority);
  
  class_< std::vector<JobStatus> >("vector_JobStatus")
    .def(vector_indexing_suite< std::vector<JobStatus> >());
    
  // Jobs and vector
  class_<TransferJobs>("TransferJobs", init<>())
    .def_readonly("JOB_ID", &TransferJobs::JOB_ID)
    .def_readonly("JOB_STATE", &TransferJobs::JOB_STATE)
    .def_readonly("CANCEL_JOB", &TransferJobs::CANCEL_JOB)
    .def_readonly("JOB_PARAMS", &TransferJobs::JOB_PARAMS)
    .def_readonly("SOURCE", &TransferJobs::SOURCE)
    .def_readonly("DEST", &TransferJobs::DEST)
    .def_readonly("SOURCE_SE", &TransferJobs::SOURCE_SE)
    .def_readonly("DEST_SE", &TransferJobs::DEST_SE)
    .def_readonly("USER_DN", &TransferJobs::USER_DN)
    .def_readonly("AGENT_DN", &TransferJobs::AGENT_DN)
    .def_readonly("USER_CRED", &TransferJobs::USER_CRED)
    .def_readonly("CRED_ID", &TransferJobs::CRED_ID)
    .def_readonly("VOMS_CRED", &TransferJobs::VOMS_CRED)
    .def_readonly("VO_NAME", &TransferJobs::VO_NAME)
    .def_readonly("SE_PAIR_NAME", &TransferJobs::SE_PAIR_NAME)
    .def_readonly("REASON", &TransferJobs::REASON)
    .def_readonly("SUBMIT_TIME", &TransferJobs::SUBMIT_TIME)
    .def_readonly("FINISH_TIME", &TransferJobs::FINISH_TIME)
    .def_readonly("PRIORITY", &TransferJobs::PRIORITY)
    .def_readonly("SUBMIT_HOST", &TransferJobs::SUBMIT_HOST)
    .def_readonly("MAX_TIME_IN_QUEUE", &TransferJobs::MAX_TIME_IN_QUEUE)
    .def_readonly("SPACE_TOKEN", &TransferJobs::SPACE_TOKEN)
    .def_readonly("STORAGE_CLASS", &TransferJobs::STORAGE_CLASS)
    .def_readonly("MYPROXY_SERVER", &TransferJobs::MYPROXY_SERVER)
    .def_readonly("SRC_CATALOG", &TransferJobs::SRC_CATALOG)
    .def_readonly("SRC_CATALOG_TYPE", &TransferJobs::SRC_CATALOG_TYPE)
    .def_readonly("DEST_CATALOG", &TransferJobs::DEST_CATALOG)
    .def_readonly("DEST_CATALOG_TYPE", &TransferJobs::DEST_CATALOG_TYPE)
    .def_readonly("INTERNAL_JOB_PARAMS", &TransferJobs::INTERNAL_JOB_PARAMS)
    .def_readonly("OVERWRITE_FLAG", &TransferJobs::OVERWRITE_FLAG)
    .def_readonly("JOB_FINISHED", &TransferJobs::JOB_FINISHED)
    .def_readonly("SOURCE_SPACE_TOKEN", &TransferJobs::SOURCE_SPACE_TOKEN)
    .def_readonly("SOURCE_TOKEN_DESCRIPTION", &TransferJobs::SOURCE_TOKEN_DESCRIPTION)
    .def_readonly("COPY_PIN_LIFETIME", &TransferJobs::COPY_PIN_LIFETIME)
    .def_readonly("LAN_CONNECTION", &TransferJobs::LAN_CONNECTION)
    .def_readonly("FAIL_NEARLINE", &TransferJobs::FAIL_NEARLINE)
    .def_readonly("CHECKSUM_METHOD", &TransferJobs::CHECKSUM_METHOD);
  
  class_< std::vector<TransferJobs> >("vector_TransferJobs")
    .def(vector_indexing_suite< std::vector<TransferJobs> >());
  
  // File transfer status and vector
  class_<FileTransferStatus>("FileTransferStatus", init<>())
    .def_readonly("logicalName", &FileTransferStatus::logicalName)
    .def_readonly("sourceSURL", &FileTransferStatus::sourceSURL)
    .def_readonly("destSURL", &FileTransferStatus::destSURL)
    .def_readonly("transferFileState", &FileTransferStatus::transferFileState)
    .def_readonly("numFailures", &FileTransferStatus::numFailures)
    .def_readonly("reason", &FileTransferStatus::reason)
    .def_readonly("reason_class", &FileTransferStatus::reason_class)
    .def_readonly("finish_time", &FileTransferStatus::finish_time)
    .def_readonly("start_time", &FileTransferStatus::start_time)
    .def_readonly("error_scope", &FileTransferStatus::error_scope)
    .def_readonly("error_phase", &FileTransferStatus::error_phase);
  
  class_< std::vector<FileTransferStatus> >("vector_FileTransferStatus")
    .def(vector_indexing_suite< std::vector<FileTransferStatus> >());
}
