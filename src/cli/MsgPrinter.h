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
#include <vector>
#include <map>

#include <boost/property_tree/ptree.hpp>

namespace fts3 {
namespace cli {

using namespace std;
using namespace boost::property_tree;

class MsgPrinter {

public:

	void endpoint(string endpoint);
	void service_version(string version);
	void service_interface(string interface);
	void service_schema(string schema);
	void service_metadata(string metadata);
	void client_version(string version);
	void client_interface(string interface);

	void wrong_endpoint_format(string endpoint);
	void missing_parameter(string name);
	void version(string version);
	void cancelled_job(string job_id);
	void error_msg(string msg);
	void gsoap_error_msg(string msg);
	void status(string status);

	void job_status(JobStatus js);
	void job_summary(JobSummary js);
	void file_list(vector<string> values);


	MsgPrinter();
	virtual ~MsgPrinter();

	void setVerbose(bool verbose) {
		this->verbose = verbose;
	}

	void setJson(bool json) {
		this->json = json;
	}


private:

	ptree getItem (map<string, string> values);

	void put (ptree&root, string name, map<string, string>& object);

	void addToArray(ptree& root, string name, map<string, string>& object);

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
