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
 * ListVOManager.h
 *
 *  Created on: Mar 14, 2012
 *      Author: Michal Simon
 */

#ifndef LISTVOMANAGER_H_
#define LISTVOMANAGER_H_

#include "CliBase.h"
#include <string>

using namespace std;

namespace fts3 { namespace cli {

class VoNameCli: virtual public CliBase {
public:

	/**
	 * Default Constructor.
	 *
	 * @param pos - if true VONAME is market as both: hidden and positional, otherwise it is a tool specific option
	 *
	 */
	VoNameCli(bool pos = true);

	/**
	 * Destructor.
	 */
	virtual ~VoNameCli();

	/**
	 * Gives the instruction how to use the command line tool.
	 *
	 * @return a string with instruction on how to use the tool
	 */
	string getUsageString(string tool);

	/**
	 * Gets a vector with job IDs.
	 *
	 * @return if job IDs were given as command line parameters a
	 * 			vector containing job IDs otherwise an empty vector
	 */
	string getVoName();
};

}
}

#endif /* LISTVOMANAGER_H_ */
