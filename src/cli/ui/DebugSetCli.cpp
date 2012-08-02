/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * DebugSetCli.h
 *
 *  Created on: Aug 2, 2012
 *      Author: Michał Simon
 */

#include "DebugSetCli.h"

using namespace fts3::cli;

const string DebugSetCli::ON = "on";
const string DebugSetCli::OFF = "off";

DebugSetCli::DebugSetCli() {

	// add hidden options
	hidden.add_options()
			("debug", value<string>(), "Set debug mode (on/off).")
			;

	// add positional (those used without an option switch) command line options
	// the 'debug' should be the first positional parameter
	positional_options_description tmp;
	tmp.add("debug", 1);
	for (int i = 0; i < p.max_total_count(); i++)
		tmp.add(p.name_for_position(i).c_str(), i + 1);

	p = tmp;
}

DebugSetCli::~DebugSetCli() {

}

GSoapContextAdapter* DebugSetCli::validate() {

	// do the standard validation
	if (!CliBase::validate()) return 0;

	// set the debug mode
	if (vm.count("debug")) {
		string str_mode = vm["debug"].as<string>();

		if (str_mode == ON) {
			mode = true;
		} else  if (str_mode == OFF){
			mode = false;
		} else {
			cout << "Debug mode has to be specified (on/off)!" << endl;
			return 0;
		}

	} else {
		cout << "Debug mode has to be specified (on/off)!" << endl;
		return 0;
	}

	if (!vm.count("source")) {
		cout << "At least one SE name must be provided!" << endl;
		return 0;
	}

	return ctx;
}

string DebugSetCli::getUsageString(string tool) {
	return "Usage: " + tool + " [options] MODE SOURCE DEST";
}
