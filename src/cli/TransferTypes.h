/*
 * TransferJob.h
 *
 *  Created on: Sep 7, 2012
 *      Author: simonm
 */

#ifndef TRANSFERJOB_H_
#define TRANSFERJOB_H_

#include <string>
#include <vector>
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

/**
 * Job element (single file)
 */
struct File {

	File () {}

	File (
			vector<string> s,
			vector<string> d,
			vector<string> c = vector<string>(),
			optional<long int> fs = optional<long int>(),
			optional<string> m = optional<string>(),
			optional<string> ss = optional<string>()) {

		sources = s;
		destinations = d;
		checksums = c;
		file_size = fs;
		metadata = m;
		selection_strategy = ss;
	}

	/// the source files (replicas)
	vector<string> sources;
	/// the destination files (the same SE different protocols)
	vector<string> destinations;
	/// source selection strategy
	optional<string> selection_strategy;
	/// checksum (multiple checksums in case of protocols that don't support adler32)
	vector<string> checksums;
	/// file size
	optional<long int> file_size;
	/// metadata
	optional<string> metadata;
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
