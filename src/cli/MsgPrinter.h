/*
 * MsgPrinter.h
 *
 *  Created on: Dec 11, 2012
 *      Author: simonm
 */

#ifndef MSGPRINTER_H_
#define MSGPRINTER_H_

#include "TransferTypes.h"

#include <string>
#include <map>

#include <boost/property_tree/ptree.hpp>

namespace fts3 {
namespace cli {

using namespace std;
using namespace boost::property_tree;

class MsgPrinter {

public:

	const string wrong_endpoint_format(string endpoint);
	const string missing_parameter(string name);
	const string version(string version);
	const string cancelled_job(string job_id);
	const string error_msg(string msg);
	const string status(string status);

	const string job_status(JobStatus js);
	const string job_summary(JobSummary js);

	MsgPrinter();
	virtual ~MsgPrinter();

	void operator() (const string (MsgPrinter::*msg)(string), string subject);

	void operator() (const string (MsgPrinter::*msg)(JobStatus), JobStatus subject);

	void operator() (const string (MsgPrinter::*msg)(JobSummary), JobSummary subject);

	void setVerbose(bool verbose) {
		this->verbose = verbose;
	}

	void setJson(bool json) {
		this->json = json;
	}


private:

	void put (string path, map<string, string> values);

	ptree getItem (map<string, string> values);

	///
	bool verbose;
	///
	bool json;
	///
	ptree json_out;

};

} /* namespace server */
} /* namespace fts3 */
#endif /* MSGPRINTER_H_ */
