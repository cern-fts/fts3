/*
 * TransferJob.h
 *
 *  Created on: Sep 7, 2012
 *      Author: simonm
 */

#ifndef TRANSFERJOB_H_
#define TRANSFERJOB_H_

#include <string>
#include <boost/optional/optional.hpp>
#include <boost/tuple/tuple.hpp>

namespace fts3 { namespace cli {

using namespace std;
using namespace boost;

typedef tuple< string, string, optional<string>, optional<int>, optional<string> > JobElement;

enum ElementMember{
	SOURCE,
	DESTINATION,
	CHECKSUM,
	FILE_SIZE,
	FILE_METADATA
};

struct JobStatus {

	JobStatus() {};

	JobStatus(string jobId, string jobStatus, string clientDn, string reason, string voName, long submitTime, int numFiles, int priority) :
		jobId(jobId),
		jobStatus(jobStatus),
		clientDn(clientDn),
		reason(reason),
		voName(voName),
		submitTime(submitTime),
		numFiles(numFiles),
		priority(priority) {

	};

	JobStatus (const JobStatus& status) :
		jobId(status.jobId),
		jobStatus(status.jobStatus),
		clientDn(status.clientDn),
		reason(status.reason),
		voName(status.voName),
		submitTime(status.submitTime),
		numFiles(status.numFiles),
		priority(status.priority) {

	};

	string jobId;
	string jobStatus;
	string clientDn;
	string reason;
	string voName;
	long submitTime;
	int numFiles;
	int priority;
};

struct JobSummary {

	JobSummary() {};

	JobSummary(
			JobStatus status,
			int numActive,
			int numCanceled,
			int numFailed,
			int numFinished,
			int numSubmitted,
			int numReady
		) :
			status(status),
			numActive(numActive),
			numCanceled(numCanceled),
			numFailed(numFailed),
			numFinished(numFinished),
			numSubmitted(numSubmitted),
			numReady(numReady) {
	};

	/// tns3__TransferJobSummary fields
	JobStatus status;
	int numActive;
	int numCanceled;
	int numFailed;
	int numFinished;
	int numSubmitted;

	/// tns3__TransferJobSummary2 fields
	int numReady;
};

}
}

#endif /* TRANSFERJOB_H_ */
