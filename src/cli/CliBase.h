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

class CliBase {
public:
	CliBase();
	virtual ~CliBase();

	void initCli(int ac, char* av[]);
	bool printHelp();
	bool printVersion();
	bool isVerbose();
	bool isQuite();
	string getService();
	string getSource();
	string getDestination();

protected:

	string discoverService();

	variables_map vm;
	options_description basic;
	options_description cli_options;
	positional_options_description p;

	string endpoint;
};

#endif /* CLIBASE_H_ */
