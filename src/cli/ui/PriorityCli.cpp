/*
 * PriorityCli.cpp
 *
 *  Created on: Dec 20, 2012
 *      Author: simonm
 */

#include "PriorityCli.h"

namespace fts3 {
namespace cli {

PriorityCli::PriorityCli() {
	// add commandline options specific for fts3-set-blk
	hidden.add_options()
			("job_id", value<string>(&job_id), "Specify subject type (se/dn)")
			("priority", value<int>(&priority), "Subject name.")
			;

	// add positional (those used without an option switch) command line options
	p.add("job_id", 1);
	p.add("priority", 1);
}

PriorityCli::~PriorityCli() {
}

optional<GSoapContextAdapter&> PriorityCli::validate(bool init) {

	// do the standard validation
	if (!CliBase::validate(init).is_initialized()) return optional<GSoapContextAdapter&>();

	if (priority < 1 || priority > 5) {
		cout << "The priority has to take a value in range of 1 to 5" << endl;
		return 0;
	}

	return *ctx;
}

string PriorityCli::getUsageString(string tool) {
	return "Usage: " + tool + " [options] JOB_ID PRIORITY";
}

} /* namespace cli */
} /* namespace fts3 */
