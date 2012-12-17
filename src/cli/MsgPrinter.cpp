/*
 * MsgPrinter.cpp
 *
 *  Created on: Dec 11, 2012
 *      Author: simonm
 */

#include "MsgPrinter.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

#include <vector>
#include <utility>
#include <sstream>

namespace fts3 {
namespace cli {

using namespace boost;

const string MsgPrinter::job_status(string job_id, string status) {

	if (!json) {
		stringstream ss;
		ss << "job " << job_id << " : " << status << endl;
		return ss.str();
	}

	optional<ptree&> job = json_out.get_child_optional("job");
	if (job.is_initialized()) {
		ptree item;
		item.put("job_id", job_id);
		item.put("status", status);
		job.get().push_back(make_pair("", item));
	} else {
		json_out.put("job..job_id", job_id);
		json_out.put("job..status", status);
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

MsgPrinter::MsgPrinter() : verbose(false), json(false) {

}

MsgPrinter::~MsgPrinter() {

}

void MsgPrinter::operator() (const string (MsgPrinter::*msg)(string), string subject) {

	cout << (this->*msg)(subject);
}


} /* namespace server */
} /* namespace fts3 */
