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
 * GetCfgCli.cpp
 *
 *  Created on: Jul 25, 2012
 *      Author: simonm
 */

#include "GetCfgCli.h"

using namespace fts3::cli;

GetCfgCli::GetCfgCli() {

	specific.add_options()
					("name,n", value<string>(), "Restrict to specific symbolic (configuration) name.")
					;
}

GetCfgCli::~GetCfgCli() {
}

string GetCfgCli::getUsageString(string tool) {
	return "Usage: " + tool + " [options] [STANDALONE_CFG | SOURCE DESTINATION]";
}

string GetCfgCli::getName() {

	if (vm.count("name")) {
		return vm["name"].as<string>();
	}

	return string();
}

//optional<GSoapContextAdapter&> GetCfgCli::validate(bool init) {
//
//	if (!CliBase::validate(init).is_initialized()) return optional<GSoapContextAdapter&>();
//
//	bool standalone = !getSource().empty();
//	bool pair = standalone && !getDestination().empty();
//
//	if ( (standalone || pair) && !getName().empty()) {
//		throw string("You may specify either a stand alone configuration, pair configuration or the symbolic name for querying!");
//	}
//
//	return *ctx;
//}

