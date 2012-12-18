/*
 * MsgPrinter.cpp
 *
 *  Created on: Dec 11, 2012
 *      Author: simonm
 */

#include "MsgPrinter.h"

#include "common/JobStatusHandler.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>

#include <utility>
#include <sstream>

namespace fts3 {
namespace cli {

using namespace boost;
using namespace boost::assign;
using namespace fts3::common;

const string MsgPrinter::cancelled_job(string job_id) {

	if (!json) {
		stringstream ss;
		ss << "job " << job_id << " : " << JobStatusHandler::FTS3_STATUS_CANCELED << endl;
		return ss.str();
	}

	map<string, string> object = map_list_of ("job_id", job_id) ("status", JobStatusHandler::FTS3_STATUS_CANCELED);

	addToArray(
			json_out,
			"job",
			object
		);


	return string();
}

const string MsgPrinter::missing_parameter(string name) {

	if (!json) {
		stringstream ss;
		ss << "missing parameter : " << name << endl;
		return ss.str();
	}

	json_out.put("missing_parameter", name);

	return string();
}

const string MsgPrinter::wrong_endpoint_format(string endpoint) {

	if (!json) {
		stringstream ss;
		ss << "wrongly formated endpoint : " << endpoint << endl;
		return ss.str();
	}

	json_out.put("wrong_format.endpoint", endpoint);

	return string();
}

const string MsgPrinter::version(string version) {

	if (!json) {
		stringstream ss;
		ss << "version : " << version << endl;
		return ss.str();
	}

	json_out.put("version", version);

	return string();
}

const string MsgPrinter::status(string status) {

	if (!json) return status;

	json_out.put("status", status);

	return string();
}

const string MsgPrinter::error_msg(string msg) {

	if (!json) {
		stringstream ss;
		ss << "error : " << msg << endl;
		return ss.str();
	}

	json_out.put("error.message", msg);

	return string();
}

const string MsgPrinter::job_status(JobStatus js) {

	char time_buff[20];
	strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&js.submitTime));

	if (!json) {
		stringstream ss;
		ss << "Request ID : " << js.jobId << endl;
		ss << "Status : " << js.jobStatus << endl;

		// if not verbose return
		if (!verbose) return ss.str();

		ss << "Client DN : " << js.clientDn << endl;
		ss << "Reason : " << (js.reason.empty() ? "<None>" : js.reason) << endl;
		ss << "Submission time : " << time_buff << endl;
		ss << "Files : " << js.numFiles << endl;
	    ss << "Priority : " << js.priority << endl;
	    ss << "VOName : " << js.voName << endl;

		return ss.str();
	}

	map<string, string> object;

	if (verbose) {
		object = map_list_of
				("job_id", js.jobId)
				("status", js.jobStatus)
				("dn", js.clientDn)
				("reason", js.reason.empty() ? "<None>" : js.reason)
				("submision_time", time_buff)
				("files_count", lexical_cast<string>(js.numFiles))
				("priority", lexical_cast<string>(js.priority))
				("vo", js.voName)
				;
	} else {
		object = map_list_of ("job_id", js.jobId) ("status", js.jobStatus);
	}

	addToArray(json_out, "job", object);

	return string();
}

const string MsgPrinter::job_summary(JobSummary js) {

	if (!json) {

		stringstream ss;

		ss << job_status(js.status);
		ss << "\tActive: " << js.numActive << endl;
		ss << "\tReady: " << js.numReady << endl;
		ss << "\tCanceled: " << js.numCanceled << endl;
		ss << "\tFinished: " << js.numFinished << endl;
		ss << "\tSubmitted: " << js.numSubmitted << endl;
		ss << "\tFailed: " << js.numFailed << endl;

		return ss.str();
	}

	char time_buff[20];
	strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&js.status.submitTime));

	map<string, string> object = map_list_of
			("job_id", js.status.jobId)
			("status", js.status.jobStatus)
			("dn", js.status.clientDn)
			("reason", js.status.reason.empty() ? "<None>" : js.status.reason)
			("submision_time", time_buff)
			("vo", js.status.voName)
			("active", lexical_cast<string>(js.numActive))
			("ready", lexical_cast<string>(js.numReady))
			("canceled", lexical_cast<string>(js.numCanceled))
			("finished", lexical_cast<string>(js.numFinished))
			("submitted", lexical_cast<string>(js.numSubmitted))
			("failed", lexical_cast<string>(js.numFailed))
			;

	addToArray(json_out, "job", object);

	return string();
}

const string MsgPrinter::file_list(vector<string> values) {

	enum {
		SOURCE,
		DESTINATION,
		STATE,
		RETRIES,
		REASON,
		DURATION
	};

	if (!json) {

		stringstream ss;

		ss << "  Source:      " << values[SOURCE] << endl;
		ss << "  Destination: " << values[DESTINATION] << endl;
		ss << "  State:       " << values[STATE] << endl;;
		ss << "  Retries:     " << values[RETRIES] << endl;
		ss << "  Reason:      " << values[REASON] << endl;
		ss << "  Duration:    " << values[DURATION] << endl;

		return ss.str();
	}

	map<string, string> object =
			map_list_of
			("source", values[SOURCE])
			("destination", values[DESTINATION])
			("state", values[STATE])
			("retries", values[RETRIES])
			("reason", values[REASON])
			("duration", values[DURATION])
			;

	ptree& job = json_out.get_child("job");
	ptree::iterator it = job.begin();
	addToArray(it->second, "files", object);

	return string();
}

MsgPrinter::MsgPrinter() : verbose(false), json(false) {

}

MsgPrinter::~MsgPrinter() {

}

ptree MsgPrinter::getItem(map<string, string> values) {

	ptree item;

	map<string, string>::iterator it;
	for (it = values.begin(); it != values.end(); it++) {
		item.put(it->first, it->second);
	}

	return item;
}

void MsgPrinter::put (ptree& root, string name, map<string, string>& object) {
	map<string, string>::iterator it;
	for (it = object.begin(); it != object.end(); it++) {
		root.put(name + it->first, it->second);
	}
}

void MsgPrinter::addToArray(ptree& root, string name, map<string, string>& object) {

	static const string array_sufix = "..";

	optional<ptree&> child = json_out.get_child_optional(name);
	if (child.is_initialized()) {
		child.get().push_front(
				make_pair("", getItem(object))
			);
	} else {
		put(root, name + array_sufix, object);
	}
}


} /* namespace server */
} /* namespace fts3 */
