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
#include <boost/regex.hpp>

#include <utility>
#include <sstream>
#include <iostream>

namespace fts3 {
namespace cli {

using namespace boost;
using namespace boost::assign;
using namespace fts3::common;

void MsgPrinter::cancelled_job(string job_id) {

	if (!json) {
		cout << "job " << job_id << " : " << JobStatusHandler::FTS3_STATUS_CANCELED << endl;
		return;
	}

	map<string, string> object = map_list_of ("job_id", job_id) ("status", JobStatusHandler::FTS3_STATUS_CANCELED);

	addToArray(
			json_out,
			"job",
			object
		);
}

void MsgPrinter::missing_parameter(string name) {

	if (!json) {
		cout << "missing parameter : " << name << endl;
		return;
	}

	json_out.put("missing_parameter", name);
}
void MsgPrinter::wrong_endpoint_format(string endpoint) {

	if (!json) {
		cout << "wrongly formated endpoint : " << endpoint << endl;
		return;
	}

	json_out.put("wrong_format.endpoint", endpoint);

	return;
}

void MsgPrinter::version(string version) {

	if (!json) {
		cout << "version : " << version << endl;
		return;
	}

	json_out.put("version", version);
}

void MsgPrinter::status(string status) {

	if (!json) {
		cout << status << endl;
		return;
	}

	json_out.put("status", status);
}

void MsgPrinter::error_msg(string msg) {

	if (!json) {
		cout << "error : " << msg << endl;
		return;
	}

	json_out.put("error.message", msg);
}

void MsgPrinter::gsoap_error_msg(string msg) {

	if (!json) {
		cout << "error : " << msg << endl;
		return;
	}

	regex re ("\"(.+)\"\nDetail: (.+)\n");
	smatch what;
	regex_match(msg, what, re, match_extra);

	json_out.put("error.message", what[1]);
	json_out.put("error.detail", what[2]);
}

void MsgPrinter::job_status(JobStatus js) {

	char time_buff[20];
	strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&js.submitTime));

	if (!json) {
		cout << "Request ID : " << js.jobId << endl;
		cout << "Status : " << js.jobStatus << endl;

		// if not verbose return
		if (!verbose) return;

		cout << "Client DN : " << js.clientDn << endl;
		cout << "Reason : " << (js.reason.empty() ? "<None>" : js.reason) << endl;
		cout << "Submission time : " << time_buff << endl;
		cout << "Files : " << js.numFiles << endl;
	    cout << "Priority : " << js.priority << endl;
	    cout << "VOName : " << js.voName << endl;

		return;
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
}

void MsgPrinter::job_summary(JobSummary js) {

	if (!json) {
		job_status(js.status);
		cout << "\tActive: " << js.numActive << endl;
		cout << "\tReady: " << js.numReady << endl;
		cout << "\tCanceled: " << js.numCanceled << endl;
		cout << "\tFinished: " << js.numFinished << endl;
		cout << "\tSubmitted: " << js.numSubmitted << endl;
		cout << "\tFailed: " << js.numFailed << endl;
		return;
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
}

void MsgPrinter::file_list(vector<string> values) {

	enum {
		SOURCE,
		DESTINATION,
		STATE,
		RETRIES,
		REASON,
		DURATION
	};

	if (!json) {
		cout << "  Source:      " << values[SOURCE] << endl;
		cout << "  Destination: " << values[DESTINATION] << endl;
		cout << "  State:       " << values[STATE] << endl;;
		cout << "  Retries:     " << values[RETRIES] << endl;
		cout << "  Reason:      " << values[REASON] << endl;
		cout << "  Duration:    " << values[DURATION] << endl;
		return;
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
}

MsgPrinter::MsgPrinter() : verbose(false), json(false) {

}

MsgPrinter::~MsgPrinter() {
	if (json)
		write_json(cout, json_out);
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
