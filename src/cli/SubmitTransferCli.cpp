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
#include "GsoapStubs.h"
#include "SrvManager.h"

#include <iostream>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

using namespace boost::algorithm;
using namespace boost;
using namespace fts::cli;

SubmitTransferCli::SubmitTransferCli() {

	FTS3_PARAM_GRIDFTP = "gridftp";
	FTS3_PARAM_MYPROXY = "myproxy";
	FTS3_PARAM_DELEGATIONID = "delegationid";
	FTS3_PARAM_SPACETOKEN = "spacetoken";
	FTS3_PARAM_SPACETOKEN_SOURCE = "source_spacetoken";
	FTS3_PARAM_COPY_PIN_LIFETIME = "copy_pin_lifetime";
	FTS3_PARAM_LAN_CONNECTION = "lan_connection";
	FTS3_PARAM_FAIL_NEARLINE = "fail_nearline";
	FTS3_PARAM_OVERWRITEFLAG = "overwrite";
	FTS3_PARAM_CHECKSUM_METHOD = "checksum_method";

	checksum = false;

	specific.add_options()
			("source", value<string>(), "Specify source site name.")
			("destination", value<string>(), "Specify destination site name.")
			("blocking,b", "Blocking mode, wait until the operation completes.")
			("file,f", value<string>(&cfg_file), "Name of a configuration file.")
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

	hidden.add_options()
			("checksum", value<string>(), "Specify checksum algorithm and value (ALGORITHM:1234af).")
			;

	p.add("source", 1);
	p.add("destination", 1);
	p.add("checksum", 1);

	cli_options.add(specific).add(hidden);
	visible.add(specific);
}

SubmitTransferCli::~SubmitTransferCli() {

}

void SubmitTransferCli::initCli(int ac, char* av[]) {
	CliBase::initCli(ac, av);

	if (vm.count("id")) {
		delegate = true;
	}
}

string SubmitTransferCli::getSource() {

	if (vm.count("source")) {
		return vm["source"].as<string>();
	}
	return "";
}

string SubmitTransferCli::getDestination() {

	if (vm.count("destination")) {
		return vm["destination"].as<string>();
	}
	return "";
}

bool SubmitTransferCli::createJobElements() {

    ifstream ifs(cfg_file.c_str());
    if (ifs) {

    	int lineCount = 0;
    	string line;
    	char_separator<char> sep(" ");
    	int size;
    	do {
    		lineCount++;
    		getline(ifs, line);

    		tokenizer< char_separator<char> > tokens(line, sep);
    		tokenizer< char_separator<char> >::iterator it;
    		size = 0;

    		string elem[3];
    		for(it = tokens.begin(); it != tokens.end() && size < 3; it++) {
    			string s = *it;
    			trim(s);
    			if (!s.empty()) {
    				elem[size] = s;
    				size++;
    			}
    		}

    		if (size == 0) continue;

    		if (size == 1) {
    			cout << "submit: in line " << lineCount << ": Destination is missing." << endl;
    			continue;
    		}

    		if (size == 3) {
    			int colon = elem[2].find(":");
   				if (colon == string::npos || colon == 0 || colon == elem[2].size() - 1) {
   					cout << "submit: in line " << lineCount << ": checksum format is not valid (ALGORITHM:1234af)." << endl;
   					continue;
   				}
    			checksum = true;
    		}

    		Task t = {elem[0], elem[1], elem[2]};
        	tasks.push_back(t);

    	} while (!ifs.eof());

    } else {

    	string checksum;
    	if (vm.count("checksum")) {
    		checksum = vm["checksum"].as<string>();
			int colon = checksum.find(":");
			if (colon == string::npos || colon == 0 || colon == checksum.size() - 1) {
				cout << "Checksum format is not valid (ALGORITHM:1234af)." << endl;
				return false;
			}
			checksum = true;
    	}

    	Task t = {getSource(), getDestination(), checksum};
    	tasks.push_back(t);
    }

    if (tasks.empty()) {
    	cout << "No transfer job is specified." << endl;
    	return false;
    }

    return true;
}

vector<transfer__TransferJobElement * > SubmitTransferCli::getJobElements(soap* soap) {

	vector<transfer__TransferJobElement * > jobElements;

	transfer__TransferJobElement* element;
	vector<Task>::iterator it;

	for (it = tasks.begin(); it < tasks.end(); it++) {
		element = soap_new_transfer__TransferJobElement(soap, -1);
		element->source = soap_new_std__string(soap, -1);
		element->dest = soap_new_std__string(soap, -1);
		*element->source = it->src;
		*element->dest = it->dest;
		jobElements.push_back(element);
	}

	return jobElements;
}

vector<transfer__TransferJobElement2 * > SubmitTransferCli::getJobElements2(soap* soap) {

	vector<transfer__TransferJobElement2 * > jobElements;

	transfer__TransferJobElement2* element;
	vector<Task>::iterator it;

	for (it = tasks.begin(); it < tasks.end(); it++) {
		element = soap_new_transfer__TransferJobElement2(soap, -1);
		element->source = soap_new_std__string(soap, -1);
		element->dest = soap_new_std__string(soap, -1);
		element->checksum = soap_new_std__string(soap, -1);

		*element->source = it->src;
		*element->dest = it->dest;
		*element->checksum = it->checksum;
	}

	return jobElements;
}

bool SubmitTransferCli::performChecks() {

	SrvManager* manager = SrvManager::getInstance();

    // decide on delegation depending on server
	if (manager->isDelegationSupported()) {
		if (!vm.count("password")) {
	        cout << "Server supports delegation. Delegation will be used by default." << endl;
	        delegate = true;
		} else {
	        cout << "Server supports delegation, however a MyProxy pass phrase was given: will use MyProxy legacy mode." << endl;
	        delegate = false;
		}
	}

    if (vm.count("password")) {
    	password = vm["password"].as<string>();
    } else {
    	if (!delegate) {
    		password = SrvManager::getInstance()->getPassword();
    	}
    }

	// the job is specified twice using cli and in the config file
	if ((!getSource().empty() || !getDestination().empty()) && vm.count("file")) {
		cout << "You may not specify a transfer on the command line if the -f option is used." << endl;
		return false;
	}

	if (vm.count("compare-checksum") && !manager->isChecksumSupported()) {
		cout << "You have specified an optional checksum, but it is not supported by the server." << endl;
		return false;
	}

    // in the case of the -I option, delegation is required
    if (delegate && !manager->isDelegationSupported()) {
        cout <<"The server you are contacting does not support credential delegation (it is running interface version ";
        cout << /*srv_iface_ver <<*/ "). Please use the MyProxy password legacy method for this server." << endl;
        return false;
    }

    // in the case of the -K option, checksum is required
    if (vm.count("compare-checksum") && !manager->isChecksumSupported()) {
        cout <<"The server you are contacting does not support checksum checking (it is running interface version ";
        cout << /*srv_iface_ve <<*/ "). Please omit checksums for this server." << endl;
        return false;
    }

	return true;
}

transfer__TransferParams* SubmitTransferCli::getParams(soap* soap) {

	transfer__TransferParams *jobParams = soap_new_transfer__TransferParams(soap, -1);

	if (vm.count("compare-checksum")) {
		jobParams->keys.push_back(FTS3_PARAM_CHECKSUM_METHOD);
		jobParams->values.push_back("compare");
	}

	if (vm.count("overwrite")) {
		jobParams->keys.push_back(FTS3_PARAM_OVERWRITEFLAG);
		jobParams->values.push_back("Y");
	}

	if (vm.count("lan-connection")) {
		jobParams->keys.push_back(FTS3_PARAM_LAN_CONNECTION);
		jobParams->values.push_back("Y");
	}

	if (vm.count("fail-nearline")) {
		jobParams->keys.push_back(FTS3_PARAM_FAIL_NEARLINE);
		jobParams->values.push_back("Y");
	}

	if (vm.count("gparam")) {
		jobParams->keys.push_back(FTS3_PARAM_GRIDFTP);
		jobParams->values.push_back(vm["gparam"].as<string>());
	}

	if (vm.count("myproxysrv")) {
		jobParams->keys.push_back(FTS3_PARAM_MYPROXY);
		jobParams->values.push_back(vm["myproxysrv"].as<string>());
	}

	if (vm.count("id")) {
		jobParams->keys.push_back(FTS3_PARAM_DELEGATIONID);
		jobParams->values.push_back(vm["id"].as<string>());
	}

	if (vm.count("dest-token")) {
		jobParams->keys.push_back(FTS3_PARAM_SPACETOKEN);
		jobParams->values.push_back(vm["dest-token"].as<string>());
	}

	if (vm.count("source-token")) {
		jobParams->keys.push_back(FTS3_PARAM_SPACETOKEN_SOURCE);
		jobParams->values.push_back(vm["source-token"].as<string>());
	}

	if (vm.count("copy-pin-lifetime")) {
		string value = boost::lexical_cast<string>(vm["copy-pin-lifetime"].as<int>());
		jobParams->keys.push_back(FTS3_PARAM_COPY_PIN_LIFETIME);
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

string SubmitTransferCli::getUsageString() {
	return "Usage: fts3-transfer-submit [options] SOURCE DEST [CHECKSUM]";
}

