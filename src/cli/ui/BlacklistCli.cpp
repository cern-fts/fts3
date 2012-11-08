/*
 * BlacklistCli.cpp
 *
 *  Created on: Nov 7, 2012
 *      Author: simonm
 */

#include "BlacklistCli.h"

namespace fts3 {
namespace cli {

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

	// add positional (those used without an option switch) command line options
	p.add("type", 1);
	p.add("subject", 1);
	p.add("mode", 1);
}

BlacklistCli::~BlacklistCli() {
	// TODO Auto-generated destructor stub
}

optional<GSoapContextAdapter&> BlacklistCli::validate(bool init) {

	// do the standard validation
	if (!CliBase::validate(init).is_initialized()) return optional<GSoapContextAdapter&>();

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
	return "Usage: " + tool + " [options] TYPE SUBJECT MODE";
}

} /* namespace cli */
} /* namespace fts3 */
