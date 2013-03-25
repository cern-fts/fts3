/*
 * BlacklistCli.cpp
 *
 *  Created on: Nov 7, 2012
 *      Author: simonm
 */

#include "BlacklistCli.h"

#include <boost/algorithm/string.hpp>

namespace fts3 {
namespace cli {

using namespace boost::algorithm;

const string BlacklistCli::ON = "on";
const string BlacklistCli::OFF = "off";

const string BlacklistCli::SE = "se";
const string BlacklistCli::DN = "dn";

BlacklistCli::BlacklistCli() {

	// add commandline options specific for fts3-set-blk
	hidden.add_options()
			("type", value<string>(&type), "Specify subject type (se/dn)")
			("subject", value<string>(&subject), "Subject name.")
			("mode", value<string>(&mode), "Mode, either: on or off")
			;

	command_specific.add_options()
			("vo", value<string>(&vo), "The VO that is banned for the given SE")
			("status", value<string>(&status)->default_value("WAIT"), "Status of the jobs that are already in the queue (CANCEL or WAIT)")
			("timeout", value<int>(&timeout)->default_value(0), "The timeout for the jobs that are already in the queue in case if 'WAIT' status (0 means the job wont timeout)")
			;

	// add positional (those used without an option switch) command line options
	p.add("type", 1);
	p.add("subject", 1);
	p.add("mode", 1);
}

BlacklistCli::~BlacklistCli() {

}

optional<GSoapContextAdapter&> BlacklistCli::validate(bool init) {

	// do the standard validation
	if (!CliBase::validate(init).is_initialized()) return optional<GSoapContextAdapter&>();

	to_lower(mode);

	if (mode != ON && mode != OFF) {
		cout << "The mode has to be either 'on' or 'off'" << endl;
		return 0;
	}

	if (type != SE && type != DN) {
		cout << "The type has to be either 'se' or 'dn'" << endl;
		return 0;
	}

	return *ctx;
}

string BlacklistCli::getUsageString(string tool) {
	return "Usage: " + tool + " [options] COMMAND NAME ON|OFF";
}

bool BlacklistCli::printHelp(string tool) {

	// check whether the -h option was used
	if (vm.count("help")) {

		// remove the path to the executive
		size_t pos = tool.find_last_of('/');
		if( pos != string::npos) {
			tool = tool.substr(pos + 1);
		}
		// print the usage guigelines
		cout << endl << getUsageString(tool) << endl << endl;

		cout << "List of Commands:" << endl << endl;
		cout << "dn		Blacklist DN (user)" << endl;
		cout << "se [options]	Blacklist SE (to get further information use '-h')" << endl;
		cout << endl << endl;

		// print the available options
        cout << visible << endl << endl;

        // print SE command options
        if (vm.count("type") && type == "se") {
			cout << "SE options:" << endl << endl;
			cout << command_specific << endl;
        }

        return true;
    }

    return false;
}

} /* namespace cli */
} /* namespace fts3 */
