/*
 * SubmitTransferCli.cpp
 *
 *  Created on: Feb 2, 2012
 *      Author: simonm
 */

#include "SubmitTransferCli.h"
#include "ftsH.h"
#include "CliManager.h"

#include <iostream>
#include <fstream>

SubmitTransferCli::SubmitTransferCli() {

	specific.add_options()
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

	cli_options.add(specific);
}

SubmitTransferCli::~SubmitTransferCli() {

}

void SubmitTransferCli::initCli(int ac, char* av[]) {
	CliBase::initCli(ac, av);

	if (vm.count("id")) {
		delegate = true;
	}

	CliManager* manager = CliManager::getInstance();

    // decide on delegation depending on server
	if (manager->isDelegationSupported()) {
		if (!vm.count("password")) {
	        cout << "# Server supports delegation. Delegation will be used by default." << endl;
	        delegate = true;
		} else {
	        cout << "# Server supports delegation, however a MyProxy pass phrase was given: will use MyProxy legacy mode." << endl;
	        delegate = false;
		}
	}

    if (vm.count("password")) {
    	password = vm["password"].as<string>();
    } else {
    	if (!delegate) {
    		password = CliManager::getInstance()->getPassword();
    	}
    }

    /*if (!cfg_file.empty()) {
    	cfg_desc.add(cli_options);

        ifstream ifs(cfg_file.c_str());
        if (!ifs) {
            cout << "Can not open configuration file: " << cfg_file << endl;
            return 0;
        } else {
            store(parse_config_file(ifs, cfg_desc), vm);
            notify(vm);
        }
    }*/
}

bool SubmitTransferCli::performChecks() {

	CliManager* manager = CliManager::getInstance();

	// the job is not specified
	if ((getSource().empty() || getDestination().empty()) && !vm.count("file")) {
		cout << "No transfer job is specified." << endl;
		return false;
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

	CliManager* manager = CliManager::getInstance();
	transfer__TransferParams *jobParams = soap_new_transfer__TransferParams(soap, -1);

	if (vm.count("compare-checksum")) {
		jobParams->keys.push_back(manager->FTS3_PARAM_CHECKSUM_METHOD);
		jobParams->values.push_back("compare");
	}

	if (vm.count("overwrite")) {
		jobParams->keys.push_back(manager->FTS3_PARAM_OVERWRITEFLAG);
		jobParams->values.push_back("Y");
	}

	if (vm.count("lan-connection")) {
		jobParams->keys.push_back(manager->FTS3_PARAM_LAN_CONNECTION);
		jobParams->values.push_back("Y");
	}

	if (vm.count("fail-nearline")) {
		jobParams->keys.push_back(manager->FTS3_PARAM_FAIL_NEARLINE);
		jobParams->values.push_back("Y");
	}

	if (vm.count("gparam")) {
		jobParams->keys.push_back(manager->FTS3_PARAM_GRIDFTP);
		jobParams->values.push_back(vm["gparam"].as<string>());
	}

	if (vm.count("myproxysrv")) {
		jobParams->keys.push_back(manager->FTS3_PARAM_MYPROXY);
		jobParams->values.push_back(vm["myproxysrv"].as<string>());
	}

	if (vm.count("id")) {
		jobParams->keys.push_back(manager->FTS3_PARAM_DELEGATIONID);
		jobParams->values.push_back(vm["id"].as<string>());
	}

	if (vm.count("dest-token")) {
		jobParams->keys.push_back(manager->FTS3_PARAM_SPACETOKEN);
		jobParams->values.push_back(vm["dest-token"].as<string>());
	}

	if (vm.count("source-token")) {
		jobParams->keys.push_back(manager->FTS3_PARAM_SPACETOKEN_SOURCE);
		jobParams->values.push_back(vm["source-token"].as<string>());
	}

	if (vm.count("copy-pin-lifetime")) {
		string value = boost::lexical_cast<string>(vm["copy-pin-lifetime"].as<int>());
		jobParams->keys.push_back(manager->FTS3_PARAM_COPY_PIN_LIFETIME);
		jobParams->values.push_back(value);
	}

	return jobParams;
}

string SubmitTransferCli::getPassword() {
	return password;
}
