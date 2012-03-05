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
 */

/*
 * TransferStatusCli.h
 *
 *  Created on: Feb 13, 2012
 *      Author: simonm
 */

#ifndef TRANSFERSTATUSCLI_H_
#define TRANSFERSTATUSCLI_H_

#include "CliBase.h"
#include <vector>
#include <string>


namespace fts { namespace cli {

/**
 * TransferStatusCli is the command line utility used for the fts3-transfer-status tool.
 *
 * In addition to the inherited functionalities from CliBase the SubmitTransferCli class provides:
 * 		- list (-l)
 * 		- job ID (--jobid), positional parameter (passed without any switch option)
 *
 * @see CliBase
 */
class TransferStatusCli: public CliBase {
public:
	/**
	 * Default constructor.
	 *
	 * Creates the transfer-status specific command line options. Job ID is
	 * market as both: hidden and positional
	 */
	TransferStatusCli();

	/**
	 * Destructor.
	 */
	virtual ~TransferStatusCli();

	/**
	 * Gives the instruction how to use the command line tool.
	 *
	 * @return a string with instruction on how to use the tool
	 */
	string getUsageString();

	/**
	 * Gets a vector with job IDs.
	 *
	 * @return if job IDs were given as command line parameters a
	 * 			vector containing job IDs otherwise an empty vector
	 */
	vector<string> getJobIds();

	/**
	 * Check if the list mode is on.
	 *
	 * @return true if the -l option has been used
	 */
	bool list();

private:

	/**
	 * command line options specific for fts3-transfer-status
	 */
	options_description specific;

	/**
	 * hidden command line options (not printed in help)
	 */
	options_description hidden;
};

}
}

#endif /* TRANSFERSTATUSCLI_H_ */
