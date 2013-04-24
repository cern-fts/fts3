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
#include <db/generic/MonitoringDbIfce.h>

using namespace boost::python;


bool operator == (const SourceAndDestSE& a, const SourceAndDestSE& b) {
    return a.destinationStorageElement == b.destinationStorageElement &&
           a.sourceStorageElement == b.sourceStorageElement;
}



bool operator == (const ConfigAudit& a, const ConfigAudit& b) {
    return a.action == b.action &&
           a.config == b.config &&
           a.userDN == b.userDN &&
           a.when   == b.when;
}



bool operator == (const TransferFiles& a, const TransferFiles& b) {
    return a.FILE_ID == b.FILE_ID;
}



bool operator == (const TransferJobs&a, const TransferJobs& b) {
    return a.JOB_ID == b.JOB_ID;
}



bool operator == (const ReasonOccurrences& a, const ReasonOccurrences& b) {
    return a.reason == b.reason;
}



bool operator == (const SePairThroughput& a, const SePairThroughput& b) {
    return a.storageElements == b.storageElements &&
           a.duration == b.duration &&
           a.averageThroughput == b.averageThroughput;
}



bool operator == (const JobVOAndSites& a, const JobVOAndSites& b) {
    return a.destinationSite == b.destinationSite &&
           a.sourceSite == b.sourceSite &&
           a.vo == b.vo;
}



void export_types(void) {
    class_<SourceAndDestSE>("SourceAndDestSE", init<>())
        .def_readwrite("sourceStorageElement", &SourceAndDestSE::sourceStorageElement)
        .def_readwrite("destinationStorageElement", &SourceAndDestSE::destinationStorageElement);

    class_<ConfigAudit>("ConfigAudit", init<>())
        .def_readonly("when", &ConfigAudit::when)
        .def_readonly("userDN", &ConfigAudit::userDN)
        .def_readonly("config", &ConfigAudit::config)
        .def_readonly("action", &ConfigAudit::action);

    class_<TransferFiles>("TransferFiles", init<>())
        .def_readonly("FILE_ID", &TransferFiles::FILE_ID)
        .def_readonly("JOB_ID", &TransferFiles::JOB_ID)
        .def_readonly("FILE_STATE", &TransferFiles::FILE_STATE)
        .def_readonly("LOGICAL_NAME", &TransferFiles::LOGICAL_NAME)
        .def_readonly("SOURCE_SURL", &TransferFiles::SOURCE_SURL)
        .def_readonly("DEST_SURL", &TransferFiles::DEST_SURL)
        .def_readonly("AGENT_DN", &TransferFiles::AGENT_DN)
        .def_readonly("ERROR_SCOPE", &TransferFiles::ERROR_SCOPE)
        .def_readonly("ERROR_PHASE", &TransferFiles::ERROR_PHASE)
        .def_readonly("REASON_CLASS", &TransferFiles::REASON_CLASS)
        .def_readonly("REASON", &TransferFiles::REASON)
        .def_readonly("NUM_FAILURES", &TransferFiles::NUM_FAILURES)
        .def_readonly("CURRENT_FAILURES", &TransferFiles::CURRENT_FAILURES)
        .def_readonly("CATALOG_FAILURES", &TransferFiles::CATALOG_FAILURES)
        .def_readonly("PRESTAGE_FAILURES", &TransferFiles::PRESTAGE_FAILURES)
        .def_readonly("FILESIZE", &TransferFiles::FILESIZE)
        .def_readonly("CHECKSUM", &TransferFiles::CHECKSUM)
        .def_readonly("FINISH_TIME", &TransferFiles::FINISH_TIME)
        .def_readonly("INTERNAL_FILE_PARAMS", &TransferFiles::INTERNAL_FILE_PARAMS)
        .def_readonly("JOB_FINISHED", &TransferFiles::JOB_FINISHED)
        .def_readonly("VO_NAME", &TransferFiles::VO_NAME)
        .def_readonly("OVERWRITE", &TransferFiles::OVERWRITE)
        .def_readonly("DN", &TransferFiles::DN)
        .def_readonly("CRED_ID", &TransferFiles::CRED_ID)
        .def_readonly("CHECKSUM_METHOD", &TransferFiles::CHECKSUM_METHOD)
        .def_readonly("SOURCE_SPACE_TOKEN", &TransferFiles::SOURCE_SPACE_TOKEN)
        .def_readonly("DEST_SPACE_TOKEN", &TransferFiles::DEST_SPACE_TOKEN)
    	.def_readonly("SOURCE_SE", &TransferFiles::SOURCE_SE)
        .def_readonly("DEST_SE", &TransferFiles::DEST_SE);

    class_<TransferJobs>("TransferJobs", init<>())
        .def_readonly("JOB_ID", &TransferJobs::JOB_ID)
        .def_readonly("JOB_STATE", &TransferJobs::JOB_STATE)
        .def_readonly("CANCEL_JOB", &TransferJobs::CANCEL_JOB)
        .def_readonly("JOB_PARAMS", &TransferJobs::JOB_PARAMS)
        .def_readonly("SOURCE", &TransferJobs::SOURCE)
        .def_readonly("DEST", &TransferJobs::DEST)
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

    class_<ReasonOccurrences>("ReasonOccurrences", init<>())
        .def_readonly("count", &ReasonOccurrences::count)
        .def_readonly("reason", &ReasonOccurrences::reason);

    class_<SePairThroughput>("SePairThroughput", init<>())
        .def_readonly("storageElements", &SePairThroughput::storageElements)
        .def_readonly("averageThroughput", &SePairThroughput::averageThroughput)
        .def_readonly("duration", &SePairThroughput::duration);

    class_<JobVOAndSites>("JobVOAndSites", init<>())
        .def_readonly("vo", &JobVOAndSites::vo)
        .def_readonly("sourceSite", &JobVOAndSites::sourceSite)
        .def_readonly("destinationSite", &JobVOAndSites::destinationSite);
}
