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

#include <vector>
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

	optional<ptree&> job = json_out.get_child_optional("job");
	if (job.is_initialized()) {
		ptree item;
		item.put("job_id", job_id);
		item.put("status", JobStatusHandler::FTS3_STATUS_CANCELED);
		job.get().push_back(make_pair("", item));
	} else {
		json_out.put("job..job_id", job_id);
		json_out.put("job..status", JobStatusHandler::FTS3_STATUS_CANCELED);
	}

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

	map<string, string> values;

	if (verbose) {
		values = map_list_of
				("job_id", js.jobId)
				("status", js.jobStatus)
				("dn", js.clientDn)
				("reason", js.reason.empty() ? "<None>" : js.reason)
				("submision_time", time_buff)
				("vo", js.voName)
				;
	} else {
		values = map_list_of ("job_id", js.jobId) ("status", js.jobStatus);
	}

	optional<ptree&> job = json_out.get_child_optional("job");
	if (job.is_initialized()) {
		job.get().push_back(
				make_pair("", getItem(values))
			);
	} else {
		string path = "job.."; // we want job to be an array
		put(path, values);
	}

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

	map<string, string> values = map_list_of
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

	optional<ptree&> job = json_out.get_child_optional("job");
	if (job.is_initialized()) {
		job.get().push_back(
				make_pair("", getItem(values))
			);
	} else {
		string path = "job.."; // we want job to be an array
		put(path, values);
	}

	return string();
}

MsgPrinter::MsgPrinter() : verbose(false), json(false) {

}

MsgPrinter::~MsgPrinter() {

}

void MsgPrinter::operator() (const string (MsgPrinter::*msg)(string), string subject) {

	cout << (this->*msg)(subject);
}

void MsgPrinter::operator() (const string (MsgPrinter::*msg)(JobStatus), JobStatus subject) {

	cout << (this->*msg)(subject);
}

void MsgPrinter::operator() (const string (MsgPrinter::*msg)(JobSummary), JobSummary subject) {

	cout << (this->*msg)(subject);
}

void MsgPrinter::put (string path, map<string, string> values) {

	map<string, string>::iterator it;
	for (it = values.begin(); it != values.end(); it++) {
		json_out.put(path + it->first, it->second);
	}
}

ptree MsgPrinter::getItem(map<string, string> values) {

	ptree item;

	map<string, string>::iterator it;
	for (it = values.begin(); it != values.end(); it++) {
		item.put(it->first, it->second);
	}

	return item;
}


} /* namespace server */
} /* namespace fts3 */
