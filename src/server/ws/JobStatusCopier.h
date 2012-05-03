/*
 * JobStatusCopier.h
 *
 *  Created on: Mar 12, 2012
 *      Author: simonm
 */

#ifndef JOBSTATUSCOPIER_H_
#define JOBSTATUSCOPIER_H_

#include "gsoap_stubs.h"

namespace fts3 { namespace ws {

/**
 * The JobStatusClass carries out the task of copying data from JobStatus structs (generic)
 * into the gSOAP tns3__JobStatus. In future may be extended to copy and create instances
 * of other gSOAP classes.
 */
class JobStatusCopier {
public:
	/**
	 * Default constructor
	 */
	JobStatusCopier(){};

	/**
	 * Destructor
	 */
	virtual ~JobStatusCopier(){};

	/*
	 * Creates a new instance of tns3__JobStatus, and copies the data from
	 * JS status into this object.
	 *
	 * The transfer_JobStatus object is created using gSOAP memory-allocation utility, it
	 * will be garbage collected! If there is a need to delete it manually gSOAP dedicated
	 * functions should be used (in particular 'soap_unlink'!).
	 *
	 * @param JS - type of the class/structure that contains data about job status
	 * @param soap - the soap object that is serving the given request
	 * @param status - the job status that has to be copied
	 *
	 * @return a newly created tns3__JobStatus, with data copied from JS status
	 */
	template <typename JS>
	inline static tns3__JobStatus* copyJobStatus(soap* soap, JS* status) {

		tns3__JobStatus* copy = soap_new_tns3__JobStatus(soap, -1);

		copy->clientDN = soap_new_std__string(soap, -1);
		*copy->clientDN = status->clientDN;

		copy->jobID = soap_new_std__string(soap, -1);
		*copy->jobID = status->jobID;

		copy->jobStatus = soap_new_std__string(soap, -1);
		*copy->jobStatus = status->jobStatus;

		copy->reason = soap_new_std__string(soap, -1);
		*copy->reason = status->reason;

		copy->voName = soap_new_std__string(soap, -1);
		*copy->voName = status->voName;

		copy->submitTime = status->submitTime;
		copy->numFiles = status->numFiles;
		copy->priority = status->priority;

		return copy;
	}

};

}
}

#endif /* JOBSTATUSCOPIER_H_ */
