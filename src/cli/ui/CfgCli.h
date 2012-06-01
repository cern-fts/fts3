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
 * NameValueCli.h
 *
 *  Created on: Apr 2, 2012
 *      Author: Michal Simon
 */

#ifndef NAMEVALUECLI_H_
#define NAMEVALUECLI_H_

#include "CliBase.h"
#include <vector>


namespace fts3 { namespace cli {

class CfgCli: public CliBase {

public:

	/**
	 * Default constructor.
	 *
	 * Creates the command line interface for retrieving name-value pairs.
	 * The name-value pair is market as both: hidden and positional.
	 */
	CfgCli();

	/**
	 * Destructor.
	 */
	virtual ~CfgCli();

	/**
	 * Validates command line options
	 * 1. Checks the endpoint
	 * 2. If -h or -V option were used respective informations are printed
	 * 3. GSoapContexAdapter is created, and info about server requested
	 * 4. Additional check regarding server are performed
	 * 5. If verbal additional info is printed
	 *
	 * @return GSoapContexAdapter instance, or null if all activities
	 * 				requested using program options have been done.
	 */
	virtual GSoapContextAdapter* validate();

	/**
	 * Gives the instruction how to use the command line tool.
	 *
	 * @return a string with instruction on how to use the tool
	 */
	string getUsageString(string tool);

	/**
	 * Gets a vector with SE configurations.
	 *
	 * @return if SE configurations were given as command line parameters a
	 * 			vector containing the value names, otherwise an empty vector
	 */
	vector<string> getConfigurations();
};

}
}

#endif /* NAMEVALUECLI_H_ */
