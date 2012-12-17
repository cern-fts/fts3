/*
 * JobStatusHandler.h
 *
 *  Created on: Mar 9, 2012
 *      Author: simonm
 */

#ifndef JOBSTATUSHANDLER_H_
#define JOBSTATUSHANDLER_H_

#include "common/ThreadSafeInstanceHolder.h"

#include <map>
#include <string>
#include <iostream>

#include "db/generic/JobStatus.h"


using namespace std;
using namespace boost;

namespace fts3 { namespace common {

/**
 * The JobStatusHandler class takes care of job status related operations.
 *
 * The class implements Singleton design pattern (derives from ThreadSafeInstanceHolder),
 * and grands access to a map containing all state names and the corresponding
 * IDs. The class inherits after MonitorObject and is thread safe.
 */
class JobStatusHandler: public ThreadSafeInstanceHolder<JobStatusHandler> {

	friend class ThreadSafeInstanceHolder<JobStatusHandler>;

public:

	/**
	 * Possible Job statuses.
	 *
	 * Value lower than 0 indicates that the job failed, value equal to 0 indicates
	 * that the job has been finished, value greater than 0 indicates that the job
	 * is still being processed
	 */
	enum JobStatusEnum {
		FTS3_STATUS_FINISHEDDIRTY_ID = -4,
		FTS3_STATUS_CANCELED_ID = -3,
		FTS3_STATUS_UNKNOWN_ID = -2,
	    FTS3_STATUS_FAILED_ID = -1,
	    FTS3_STATUS_FINISHED_ID = 0,
	    FTS3_STATUS_SUBMITTED_ID,
	    FTS3_STATUS_READY_ID,
	    FTS3_STATUS_ACTIVE_ID
	};

	///@{
	/**
	 * names of transfer job statuses
	 */
	static const string FTS3_STATUS_FINISHEDDIRTY;
    static const string FTS3_STATUS_CANCELED;
	static const string FTS3_STATUS_UNKNOWN;
    static const string FTS3_STATUS_FAILED;
    static const string FTS3_STATUS_FINISHED;
    static const string FTS3_STATUS_SUBMITTED;
    static const string FTS3_STATUS_READY;
    static const string FTS3_STATUS_ACTIVE;
	///@}

	/**
	 * Destructor (empty).
	 */
	virtual ~JobStatusHandler() {}

	/**
	 * Check whether the given status name indicates that a transfer is ready
	 *
	 * @return true if the transfer is ready
	 */
	bool isTransferFinished(string status);

	/**
	 * Check weather the given state is valid
	 * (is one of the recognized states)
	 *
	 * @return true if the the state is valid
	 */
	bool isStatusValid(string& status);

	/**
	 * Counts how many states in the given vector are equal to given state
	 *
	 * @param STATE
	 * @param states
	 *
	 * @return
	 */
	int countInState(string status, vector<JobStatus*>& statuses);

	/**
	 *
	 */
	template <typename JS>
	static void printJobStatus(JS js){

		cout << "Request ID:\t" << js.jobId << endl;
		cout << "Status: " << js.jobStatus << endl;
		cout << "Client DN: " << js.clientDn << endl;

		if (!js.reason.empty()) {

			cout << "Reason: " << js.reason << endl;

		} else {

			cout << "Reason: <None>" << endl;
		}

		char buff[20];
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&js.submitTime));
		cout << "Submit time: " << buff << endl;
		cout << "Files: " << js.numFiles << endl;
	    cout << "Priority: " << js.priority << endl;
	    cout << "VOName: " << js.voName << endl;
	}

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
	inline tns3__JobStatus* copyJobStatus(soap* soap, JS* status) {
//
//		tns3__JobStatus* copy = soap_new_tns3__JobStatus(soap, -1);
//
//		copy->clientDN = soap_new_std__string(soap, -1);
//		*copy->clientDN = status->clientDN;
//
//		copy->jobID = soap_new_std__string(soap, -1);
//		*copy->jobID = status->jobID;
//
//		copy->jobStatus = soap_new_std__string(soap, -1);
//		*copy->jobStatus = status->jobStatus;
//
//		copy->reason = soap_new_std__string(soap, -1);
//		*copy->reason = status->reason;
//
//		copy->voName = soap_new_std__string(soap, -1);
//		*copy->voName = status->voName;
//
//		copy->submitTime = status->submitTime;
//		copy->numFiles = status->numFiles;
//		copy->priority = status->priority;
//
//		return copy;
		return 0;
	}

protected:

	/**
	 * Default constructor
	 *
	 * Private, should not be used
	 */
	JobStatusHandler();

private:

	/**
	 * Copying constructor
	 *
	 * Private, should not be used
	 */
	JobStatusHandler(JobStatusHandler const&);

	/**
	 * Assignment operator
	 *
	 * Private, should not be used
	 */
	JobStatusHandler & operator=(JobStatusHandler const&);

	/// maps job status name to job status id
	const map<string, JobStatusEnum> statusNameToId;
};


}
}
#endif /* JOBSTATUSHANDLER_H_ */
