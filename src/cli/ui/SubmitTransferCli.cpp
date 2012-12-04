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
 * SubmitTransferCli.cpp
 *
 *  Created on: Feb 2, 2012
 *      Author: simonm
 */

#include "SubmitTransferCli.h"
#include "common/JobParameterHandler.h"

#include <iostream>
#include <fstream>
#include <termios.h>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost::algorithm;
using namespace boost;
using namespace fts3::cli;
using namespace fts3::common;

SubmitTransferCli::SubmitTransferCli() {

	// by default we don't use checksum
	checksum = false;

	// add commandline options specific for fts3-transfer-submit
	specific.add_options()
			("blocking,b", "Blocking mode, wait until the operation completes.")
			("file,f", value<string>(&bulk_file), "Name of a configuration file.")
			("gparam,g", value<string>(), "Gridftp parameters.")
			("interval,i", value<int>(), "Interval between two poll operations in blocking mode.")
//			("myproxysrv,m", value<string>(), "MyProxy server to use.")
			("password,p", value<string>(), "MyProxy password to send with the job")
			("id,I", value<string>(), "Delegation with ID as the delegation identifier.")
			("expire,e", value<long>(), "Expiration time of the delegation in minutes.")
			("overwrite,o", "Overwrite files.")
			("dest-token,t", value<string>(),  "The destination space token or its description (for SRM 2.2 transfers).")
			("source-token,S", value<string>(), "The source space token or its description (for SRM 2.2 transfers).")
			("compare-checksum,K", "Compare checksums between source and destination.")
			("copy-pin-lifetime", value<int>(), "pin lifetime of the copy of the file (seconds)")
			("lan-connection", "use LAN as ConnectionType (default = WAN)")
			("fail-nearline", "fail the transfer if the file is nearline")
			("reuse,r", "enable session reuse the transfer job")
			;

	// add hidden options
	hidden.add_options()
			("checksum", value<string>(), "Specify checksum algorithm and value (ALGORITHM:1234af).")
			;

	// add positional (those used without an option switch) command line options
	p.add("checksum", 1);

}

SubmitTransferCli::~SubmitTransferCli() {

}

void SubmitTransferCli::parse(int ac, char* av[]) {

	// do the basic initialization
	CliBase::parse(ac, av);

	// check whether to use delegation
	if (vm.count("id")) {
		delegate = true;
	}
}

optional<GSoapContextAdapter&> SubmitTransferCli::validate(bool init) {

	// do the standard validation
	if (!CliBase::validate(init).is_initialized()) return optional<GSoapContextAdapter&>();

	// perform standard checks in order to determine if the job was well specified
	if(!performChecks()) return optional<GSoapContextAdapter&>();

	// prepare job elements
	if (!createJobElements()) return optional<GSoapContextAdapter&>();

	return *ctx;
}

string SubmitTransferCli::getDelegationId() {

	// check if destination was passed via command line options
	if (vm.count("id")) {
		return vm["id"].as<string>();
	}
	return "";
}

long SubmitTransferCli::getExpirationTime() {

	if (vm.count("expire")) {
		return vm["expire"].as<long>();
	}
	return 0;
}


bool SubmitTransferCli::createJobElements() {

	// first check if the -f option was used, try to open the file with bulk-job description
	ifstream ifs(bulk_file.c_str());
    if (ifs) {

    	// Parse the file
    	int lineCount = 0;
    	string line;
    	// define the seperator characters (space) for tokenizing each line
    	char_separator<char> sep(" ");
    	// read and parse the lines one by one
    	do {
    		lineCount++;
    		getline(ifs, line);

    		// split the line into tokens
    		tokenizer< char_separator<char> > tokens(line, sep);
    		tokenizer< char_separator<char> >::iterator it;

    		// we are expecting up to 3 elements in each line
    		// source, destination and optionally the checksum
    		JobElement e;

    		// the first part should be the source
    		it = tokens.begin();
    		if (it != tokens.end())
    			get<SOURCE>(e) = *it;
    		else
    			// if the line was empty continue
    			continue;

    		// the second part should be the destination
    		it++;
    		if (it != tokens.end())
    			get<DESTINATION>(e) = *it;
    		else {
    			// only one element is still not enough to define a job
    			cout << "submit: in line " << lineCount << ": Destination is missing." << endl;
    			continue;
    		}

    		// the third part should be the checksum (but its optional)
    		it++;
    		if (it != tokens.end())
    			get<CHECKSUM>(e) = *it;

    		// if the checksum algorithm has been given check if the
    		// format is correct (ALGORITHM:1234af)
    		if (get<CHECKSUM>(e)) {
    			string::size_type colon = (*get<CHECKSUM>(e)).find(":");
   				if (colon == string::npos || colon == 0 || colon == (*get<CHECKSUM>(e)).size() - 1) {
   					cout << "submit: in line " << lineCount << ": checksum format is not valid (ALGORITHM:1234af)." << endl;
   					continue;
   				}
    			checksum = true;
    		}
        	elements.push_back(e);

    	} while (!ifs.eof());

    } else {

    	// the -f was not used, so use the values passed via CLI

    	// first, if the checksum algorithm has been given check if the
    	// format is correct (ALGORITHM:1234af)
    	optional<string> checksum;
    	if (vm.count("checksum")) {
			checksum = vm["checksum"].as<string>();
			this->checksum = true;
    	}

    	// then if the source and destination have been given create a Task
    	if (!getSource().empty() && !getDestination().empty()) {
    		elements.push_back (
    				JobElement (getSource(), getDestination(), checksum)
    			);
    	}
    }

    return true;
}

vector<JobElement> SubmitTransferCli::getJobElements() {

	return elements;
}

bool SubmitTransferCli::performChecks() {

    // check if the server supports delegation
	if (ctx->isDelegationSupported()) {
		// if the user did not specify a password use delegation
		if (!vm.count("password")) {
			if (isVerbose())
				cout << "Server supports delegation. Delegation will be used by default." << endl;
	        delegate = true;
		} else {
			if (isVerbose())
				cout << "Server supports delegation, however a MyProxy pass phrase was given: will use MyProxy legacy mode." << endl;
	        delegate = false;
		}
	}

	// if the user specified the password set the value of 'password' variable
    if (vm.count("password")) {
    	password = vm["password"].as<string>();
    } else {
    	// if not, and delegation mode is not use,
    	// ask the user to give the password
    	if (!delegate) {
    		password = askForPassword();
    	}
    }

	// the job cannot be specified twice
	if ((!getSource().empty() || !getDestination().empty()) && vm.count("file")) {
		cout << "You may not specify a transfer on the command line if the -f option is used." << endl;
		return false;
	}

	// if the checksum algorithm is given the server has to be support checksum
	if (checksum && !ctx->isChecksumSupported()) {
		cout << "You have specified an optional checksum, but it is not supported by the server." << endl;
		return false;
	}

    // if the checksum is used it has to be supported by the server
    if (vm.count("compare-checksum") && !ctx->isChecksumSupported()) {
    	cout << "The server you are contacting does not support checksum checking (it is running interface version ...). Please omit checksums for this server." << endl;
		return false;
    }

    // if delegation is used it has to be supported by the server
    if (delegate && !ctx->isDelegationSupported()) {
    	cout <<
    			"The server you are contacting does not support credential delegation (it is running interface version ...). Please use the MyProxy password legacy method for this server."
    		<< endl;
    	return false;
    }

    return true;
}

string SubmitTransferCli::askForPassword() {

    termios stdt;
    // get standard command line settings
    tcgetattr(STDIN_FILENO, &stdt);
    termios newt = stdt;
    // turn off echo while typing
    newt.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt)) {
    	cout << "submit: could not set terminal attributes" << endl;
    	tcsetattr(STDIN_FILENO, TCSANOW, &stdt);
    	return "";
    }

    string pass1, pass2;

    cout << "Enter MyProxy password: ";
    cin >> pass1;
    cout << endl << "Enter MyProxy password again: ";
    cin >> pass2;
    cout << endl;

    // set the standard command line settings back
    tcsetattr(STDIN_FILENO, TCSANOW, &stdt);

    // compare passwords
    if (pass1.compare(pass2)) {
    	cout << "Entered MyProxy passwords do not match." << endl;
    	return "";
    }

    return pass1;
}

map<string, string> SubmitTransferCli::getParams() {

	map<string, string> parameters;

	// check if the parameters were set using CLI, and if yes set them

	if (vm.count("compare-checksum")) {
		parameters[JobParameterHandler::FTS3_PARAM_CHECKSUM_METHOD] = "compare";
	}

	if (vm.count("overwrite")) {
		parameters[JobParameterHandler::FTS3_PARAM_OVERWRITEFLAG] = "Y";
	}

	if (vm.count("lan-connection")) {
		parameters[JobParameterHandler::FTS3_PARAM_LAN_CONNECTION] = "Y";
	}

	if (vm.count("fail-nearline")) {
		parameters[JobParameterHandler::FTS3_PARAM_FAIL_NEARLINE] = "Y";
	}

	if (vm.count("gparam")) {
		parameters[JobParameterHandler::FTS3_PARAM_GRIDFTP] = vm["gparam"].as<string>();
	}

	if (vm.count("myproxysrv")) {
		parameters[JobParameterHandler::FTS3_PARAM_MYPROXY] = vm["myproxysrv"].as<string>();
	}

	if (vm.count("id")) {
		parameters[JobParameterHandler::FTS3_PARAM_DELEGATIONID] = vm["id"].as<string>();
	}

	if (vm.count("dest-token")) {
		parameters[JobParameterHandler::FTS3_PARAM_SPACETOKEN] = vm["dest-token"].as<string>();
	}

	if (vm.count("source-token")) {
		parameters[JobParameterHandler::FTS3_PARAM_SPACETOKEN_SOURCE] = vm["source-token"].as<string>();
	}

	if (vm.count("copy-pin-lifetime")) {
		parameters[JobParameterHandler::FTS3_PARAM_COPY_PIN_LIFETIME] = lexical_cast<string>(vm["copy-pin-lifetime"].as<int>());
	}

	if (vm.count("reuse")) {
		parameters[JobParameterHandler::FTS3_PARAM_REUSE] = "Y";
	}

	return parameters;
}

string SubmitTransferCli::getPassword() {
	return password;
}

bool SubmitTransferCli::isBlocking() {
	return vm.count("blocking");
}

string SubmitTransferCli::getUsageString(string tool) {
	return "Usage: " + tool + " [options] SOURCE DEST [CHECKSUM]";
}
