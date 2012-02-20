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
 * CliBase.h
 *
 *  Created on: Feb 2, 2012
 *      Author: simonm
 */

#ifndef CLIBASE_H_
#define CLIBASE_H_

#include <boost/program_options.hpp>

using namespace boost::program_options;
using namespace std;


namespace fts { namespace cli {

class CliBase {
public:
	CliBase();
	virtual ~CliBase();

	void initCli(int ac, char* av[]);
	bool printHelp();
	bool printVersion();
	void printGeneralInfo();
	bool isVerbose();
	bool isQuite();
	string getService();
	virtual string getUsageString() = 0;

protected:

	string discoverService();

	variables_map vm;
	options_description basic;
	options_description visible;
	options_description cli_options;
	positional_options_description p;

	string endpoint;

private:
	// service discovery
	string FTS3_CA_SD_TYPE;
	string FTS3_SD_ENV;
	string FTS3_SD_TYPE;
	string FTS3_IFC_VERSION;
	string FTS3_INTERFACE_VERSION;
};

}
}

#endif /* CLIBASE_H_ */
