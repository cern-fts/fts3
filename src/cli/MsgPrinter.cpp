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

#include <vector>
#include <utility>
#include <sstream>

namespace fts3 {
namespace cli {

using namespace boost;
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

	if (!json) {
		stringstream ss;
		ss << "Request ID : " << js.jobId << endl;
		ss << "Status : " << js.jobStatus << endl;

		// if not verbose return
		if (!verbose) return ss.str();

		ss << "Client DN : " << js.clientDn << endl;

		if (!js.reason.empty()) {

			ss << "Reason : " << js.reason << endl;

		} else {

			ss << "Reason : <None>" << endl;
		}

		char buff[20];
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&js.submitTime));
		ss << "Submission time : " << buff << endl;
		ss << "Files : " << js.numFiles << endl;
	    ss << "Priority : " << js.priority << endl;
	    ss << "VOName : " << js.voName << endl;

		return ss.str();
	}

	optional<ptree&> job = json_out.get_child_optional("job");
	if (job.is_initialized()) {
		ptree item;
		item.put("job_id", js.jobId);
		item.put("status", js.jobStatus);

		if (verbose) {
			item.put("job_id", js.jobId);
			item.put("status", js.jobId);
			item.put("dn", js.clientDn);
			item.put("reason", js.reason);

			char buff[20];
			strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&js.submitTime));
			item.put("submision_time", buff);

			item.put("vo", js.voName);
		}

		job.get().push_back(make_pair("", item));

	} else {
		json_out.put("job..job_id", js.jobId);
		json_out.put("job..status", js.jobStatus);

		if (verbose) {
			json_out.put("job..job_id", js.jobId);
			json_out.put("job..status", js.jobId);
			json_out.put("job..dn", js.clientDn);
			json_out.put("job..reason", js.reason);

			char buff[20];
			strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&js.submitTime));
			json_out.put("job..submision_time", buff);

			json_out.put("job..vo", js.voName);
		}
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


} /* namespace server */
} /* namespace fts3 */
