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
 * GetLogCli.h
 *
 *  Created on: Oct 4, 2012
 *      Author: Michal Simon
 */

#ifndef GETLOGCLI_H_
#define GETLOGCLI_H_

#include "JobIdCli.h"

using namespace std;

namespace fts3 { namespace cli {

/**
 * GetLogCli is the command line utility used for retrieving job log files.
 *
 * In addition to the inherited functionalities from JobIdCli the GetLogCli class provides:
 * 		- destination path  (--path)
 *
 * @see JobIdCli
 */
class GetLogCli : public JobIdCli {
public:

	/**
	 * Default constructor.
	 *
	 * Creates the command line interface for retrieving log files.
	 */
	GetLogCli();

	/**
	 * Destructor.
	 */
	virtual ~GetLogCli();

	/**
	 * Gets the destination path
	 *
	 * @return destination path
	 */
	string getPath();
};

}
}

#endif /* GETLOGCLI_H_ */
