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
#include "gsoap_transfer_proxy.h"
#include "SrvManager.h"
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

	// but default we don't use checksum
	checksum = false;

	// add commandline options specific for fts3-transfer-submit
	specific.add_options()
			("source", value<string>(), "Specify source site name.")
			("destination", value<string>(), "Specify destination site name.")
			("blocking,b", "Blocking mode, wait until the operation completes.")
			("file,f", value<string>(&bulk_file), "Name of a configuration file.")
			("gparam,g", value<string>(), "Gridftp parameters.")
			("interval,i", value<int>(), "Interval between two poll operations in blocking mode.")
			("myproxysrv,m", value<string>(), "MyProxy server to use.")
			("password,p", value<string>(), "MyProxy password to send with the job")
			("id,I", value<string>(), "Delegation with ID as the delegation identifier.")
			("expire,e", value<long>(), "Expiration time of the delegation in minutes.")
			("overwrite,o", "Overwrite files.")
			("dest-token,t", value<string>(),  "The destination space token or its description (for SRM 2.2 transfers).")
			("source-token,S", value<string>(), "he source space token or its description (for SRM 2.2 transfers).")
			("compare-checksum,K", "Compare checksums between source and destination.")
			("copy-pin-lifetime", value<int>(), "pin lifetime of the copy of the file (seconds)")
			("lan-connection", "use LAN as ConnectionType (default = WAN)")
			("fail-nearline", "fail the transfer if the file is nearline")
			;

	// add hidden options
	hidden.add_options()
			("checksum", value<string>(), "Specify checksum algorithm and value (ALGORITHM:1234af).")
			;

	// add positional (those used without an option switch) command line options
	p.add("source", 1);
	p.add("destination", 1);
	p.add("checksum", 1);

}

SubmitTransferCli::~SubmitTransferCli() {

}

void SubmitTransferCli::initCli(int ac, char* av[]) {

	// do the basic initialization
	CliBase::initCli(ac, av);

	// check whether to use delegation
	if (vm.count("id")) {
		delegate = true;
	}
}

string SubmitTransferCli::getSource() {

	// check if source was passed via command line options
	if (vm.count("source")) {
		return vm["source"].as<string>();
	}
	return "";
}

string SubmitTransferCli::getDestination() {

	// check if destination was passed via command line options
	if (vm.count("destination")) {
		return vm["destination"].as<string>();
	}
	return "";
}

bool SubmitTransferCli::createJobElements() {

	// first check if the -f option was used, try to open the file with bulk-job description
	ifstream ifs(bulk_file.c_str());
    if (ifs) {

    	// Parse the file
    	int lineCount = 0;
    	string line;
    	// define the seperator for tokenizing each line
    	char_separator<char> sep(" ");
    	int count;
    	// read and parse the lines one by one
    	do {
    		lineCount++;
    		getline(ifs, line);

    		// split the line into tokens
    		tokenizer< char_separator<char> > tokens(line, sep);
    		tokenizer< char_separator<char> >::iterator it;
    		count = 0;

    		// we are expecting up to 3 elements in each line
    		// source, destination and optionally the checksum
    		JobElement e;
    		// iterate over the tokens (we are interested in up to 3 tokens)
    		for(it = tokens.begin(); it != tokens.end() && count < 3; it++) {
    			string s = *it;
    			trim(s);
    			if (!s.empty()) {
    				// if the token is not empty after trimming,
    				// it is the element we are interested in
    				e[count] = s;
    				count++;
    			}
    		}

    		// if the line was empty continue
    		if (count == 0) continue;

    		// only one element is still not enough to define a job
    		if (count == 1) {
    			cout << "submit: in line " << lineCount << ": Destination is missing." << endl;
    			continue;
    		}

    		// if the checksum algorithm has been given check if the
    		// format is correct (ALGORITHM:1234af)
    		if (count == 3) {
    			string::size_type colon = e[2].find(":");
   				if (colon == string::npos || colon == 0 || colon == e[2].size() - 1) {
   					cout << "submit: in line " << lineCount << ": checksum format is not valid (ALGORITHM:1234af)." << endl;
   					continue;
   				}
    			checksum = true;
    		}
        	tasks.push_back(e);

    	} while (!ifs.eof());

    } else {

    	// the -f was not used, so use the values passed via CLI

    	// first, if the checksum algorithm has been given check if the
    	// format is correct (ALGORITHM:1234af)
    	string checksum;
    	if (vm.count("checksum")) {
    		checksum = vm["checksum"].as<string>();
			string::size_type colon = checksum.find(":");
			if (colon == string::npos || colon == 0 || colon == checksum.size() - 1) {
				cout << "Checksum format is not valid (ALGORITHM:1234af)." << endl;
				return false;
			}
			checksum = true;
    	}

    	// then if the source and destination have been given create a Task
    	if (!getSource().empty() && !getDestination().empty()) {
    		JobElement e = {getSource(), getDestination(), checksum};
    		tasks.push_back(e);
    	}
    }

    // finally check if at least one Task has been specified
    if (tasks.empty()) {
    	cout << "No transfer job is specified." << endl;
    	return false;
    }

    return true;
}

vector<tns3__TransferJobElement * > SubmitTransferCli::getJobElements(soap* soap) {

	vector<tns3__TransferJobElement * > jobElements;

	tns3__TransferJobElement* element;
	vector<JobElement>::iterator it;

	// iterate over the internal vector containing Task (job elements)
	for (it = tasks.begin(); it < tasks.end(); it++) {

		// create the job element, and set the source and destination values
		element = soap_new_tns3__TransferJobElement(soap, -1);
		element->source = soap_new_std__string(soap, -1);
		element->dest = soap_new_std__string(soap, -1);
		*element->source = it->src;
		*element->dest = it->dest;
		// push the element into the result vector
		jobElements.push_back(element);
	}

	return jobElements;
}

vector<tns3__TransferJobElement2 * > SubmitTransferCli::getJobElements2(soap* soap) {

	vector<tns3__TransferJobElement2 * > jobElements;

	tns3__TransferJobElement2* element;
	vector<JobElement>::iterator it;

	// iterate over the internal vector containing Task (job elements)
	for (it = tasks.begin(); it < tasks.end(); it++) {

		// create the job element, and set the source, destination and checksum values
		element = soap_new_tns3__TransferJobElement2(soap, -1);
		element->source = soap_new_std__string(soap, -1);
		element->dest = soap_new_std__string(soap, -1);
		element->checksum = soap_new_std__string(soap, -1);

		*element->source = it->src;
		*element->dest = it->dest;
		*element->checksum = it->checksum;

		// push the element into the result vector
		jobElements.push_back(element);
	}

	return jobElements;
}

bool SubmitTransferCli::performChecks() {

	SrvManager* manager = SrvManager::getInstance();

    // check if the server supports delegation
	if (manager->isDelegationSupported()) {
		// if the user did not specify a password use delegation
		if (!vm.count("password")) {
	        cout << "Server supports delegation. Delegation will be used by default." << endl;
	        delegate = true;
		} else {
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
	if (checksum && !manager->isChecksumSupported()) {
		cout << "You have specified an optional checksum, but it is not supported by the server." << endl;
		return false;
	}

    // if the checksum is used it has to be supported by the server
    if (vm.count("compare-checksum") && !manager->isChecksumSupported()) {
        cout <<"The server you are contacting does not support checksum checking (it is running interface version ";
        cout << /*srv_iface_ve <<*/ "). Please omit checksums for this server." << endl;
        return false;
    }

    // if delegation is used it has to be supported by the server
    if (delegate && !manager->isDelegationSupported()) {
        cout <<"The server you are contacting does not support credential delegation (it is running interface version ";
        cout << /*srv_iface_ver <<*/ "). Please use the MyProxy password legacy method for this server." << endl;
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

tns3__TransferParams* SubmitTransferCli::getParams(soap* soap) {

	tns3__TransferParams *jobParams = soap_new_tns3__TransferParams(soap, -1);

	// check if the parameters were set using CLI, and if yes set them

	if (vm.count("compare-checksum")) {
		jobParams->keys.push_back(JobParameterHandler::FTS3_PARAM_CHECKSUM_METHOD);
		jobParams->values.push_back("compare");
	}

	if (vm.count("overwrite")) {
		jobParams->keys.push_back(JobParameterHandler::FTS3_PARAM_OVERWRITEFLAG);
		jobParams->values.push_back("Y");
	}

	if (vm.count("lan-connection")) {
		jobParams->keys.push_back(JobParameterHandler::FTS3_PARAM_LAN_CONNECTION);
		jobParams->values.push_back("Y");
	}

	if (vm.count("fail-nearline")) {
		jobParams->keys.push_back(JobParameterHandler::FTS3_PARAM_FAIL_NEARLINE);
		jobParams->values.push_back("Y");
	}

	if (vm.count("gparam")) {
		jobParams->keys.push_back(JobParameterHandler::FTS3_PARAM_GRIDFTP);
		jobParams->values.push_back(vm["gparam"].as<string>());
	}

	if (vm.count("myproxysrv")) {
		jobParams->keys.push_back(JobParameterHandler::FTS3_PARAM_MYPROXY);
		jobParams->values.push_back(vm["myproxysrv"].as<string>());
	}

	if (vm.count("id")) {
		jobParams->keys.push_back(JobParameterHandler::FTS3_PARAM_DELEGATIONID);
		jobParams->values.push_back(vm["id"].as<string>());
	}

	if (vm.count("dest-token")) {
		jobParams->keys.push_back(JobParameterHandler::FTS3_PARAM_SPACETOKEN);
		jobParams->values.push_back(vm["dest-token"].as<string>());
	}

	if (vm.count("source-token")) {
		jobParams->keys.push_back(JobParameterHandler::FTS3_PARAM_SPACETOKEN_SOURCE);
		jobParams->values.push_back(vm["source-token"].as<string>());
	}

	if (vm.count("copy-pin-lifetime")) {
		string value = lexical_cast<string>(vm["copy-pin-lifetime"].as<int>());
		jobParams->keys.push_back(JobParameterHandler::FTS3_PARAM_COPY_PIN_LIFETIME);
		jobParams->values.push_back(value);
	}

	return jobParams;
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

