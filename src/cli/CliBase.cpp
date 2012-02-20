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
 * CliBase.cpp
 *
 *  Created on: Feb 2, 2012
 *      Author: simonm
 */

#include "CliBase.h"
#include  "SrvManager.h"

#include <iostream>

//#include "ServiceDiscoveryIfce.h" // check "" or <>
//#include <glite/data/glite-util.h>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>

using namespace boost;
using namespace fts::cli;


CliBase::CliBase() : visible("Allowed options") {

	FTS3_CA_SD_TYPE = "org.glite.ChannelAgent";
	FTS3_SD_ENV = "GLITE_SD_FTS_TYPE";
	FTS3_SD_TYPE = "org.glite.FileTransfer";
	FTS3_IFC_VERSION = "GLITE_FTS_IFC_VERSION";
	FTS3_INTERFACE_VERSION = "3.7.0"; // TODO where it comes from?

    basic.add_options()
			("help,h", "Print this help text and exit.")
			("quite,q", "Quiet operation.")
			("verbose,v", "Be more verbose.")
			("service,s", value<string>(), "Use the transfer service at the specified URL.")
			("version,V", "Print the version number and exit.")
			;

    cli_options.add(basic);
    visible.add(basic);
}

CliBase::~CliBase() {

}


void CliBase::initCli(int ac, char* av[]) {
	// turn off guessing, so --source is not mistaken with --source-token
	int style = command_line_style::default_style & ~command_line_style::allow_guessing;

	store(command_line_parser(ac, av).options(cli_options).positional(p).style(style).run(), vm);
	notify(vm);

	if (vm.count("service")) {
		endpoint = vm["service"].as<string>();
		// check if the endpoint has the right prefix
		if (endpoint.find("http") != 0 && endpoint.find("https") != 0 && endpoint.find("httpd") != 0) {
			// if not erase
			cout << "Wrongly formatted endpoint: " << endpoint << endl;
			endpoint.erase();
		}
	} else {
		if (!vm.count("help") && !vm.count("version")) {
			endpoint = discoverService();
		}
	}
}

bool CliBase::printHelp() {

	if (vm.count("help")) {
		cout << endl << getUsageString() << endl << endl;
        cout << visible << endl;
        return true;
    }

    return false;
}

bool CliBase::printVersion() {

	if (vm.count("version")) {
        cout << "TODO" << endl;
        return true;
    }

    return false;
}

void CliBase::printGeneralInfo() {

	SrvManager* manager = SrvManager::getInstance();

	cout << "# Using endpoint: " << getService() << endl;
	cout << "# Service version: " << manager->getVersion() << endl;
	cout << "# Interface version: " << manager->getInterface() << endl;
	cout << "# Schema version: " << manager->getSchema() << endl;
	cout << "# Service features: " << manager->getMetadata() << endl;
	cout << "# Client version: TODO" << endl;
	cout << "# Client interface version: TODO" << endl;
}

bool CliBase::isVerbose() {
	return vm.count("verbose");
}

bool CliBase::isQuite() {
	return vm.count("quite");
}

string CliBase::getService() {

	return endpoint;
}

string CliBase::discoverService() {
/*
	//string source = getSource(), destination = getDestination();
	SDService *service;
	scoped_array<char> c_strArr, errorArr;
	char *error, *c_str = 0;
	const char * srvtype;
	const char * version;
	SDException exc;
*/
	/*if (source.size() > 0 && destination.size() > 0) {

		SDServiceData datas[] =
		{
			{"source", (char*) source.c_str()},
			{"destination", (char*) destination.c_str()}
		};

		SDServiceDataList datalist = {NULL, 2, datas};

		c_str = glite_discover_service_by_data(FTS3_CA_SD_TYPE.c_str(), &datalist, &error);
		c_str_arr.reset(c_str);
		error_arr.reset(error);
	}*/
/*
	if (getenv(FTS3_SD_ENV.c_str()))
		srvtype = getenv(FTS3_SD_ENV.c_str());
	else
		srvtype = FTS3_SD_TYPE.c_str();

	if(getenv(FTS3_IFC_VERSION.c_str()))
		version = getenv(FTS3_IFC_VERSION.c_str());
	else
		version = FTS3_INTERFACE_VERSION.c_str();

	// Note: glite_discover_service will check if the service name passed
	// has an associated service of the given type (fts_srvtype), if not
	// it will check if the name refers to a site name or an host name

	c_str = glite_discover_service_by_version(srvtype, c_str, version, &error);
	c_strArr.reset(c_str);
	errorArr.reset(error);

	if (!c_str) {
        cout << "Service discovery: " << error << endl;
		return "";
	}

	service = SD_getService(c_str, &exc);
	if (!service) {
		cout << "Service discovery lookup failed for " << c_str << ": " << exc.reason << endl;
		SD_freeException(&exc);
		return "";
	}

	string tmp;
	if (service->endpoint) {
		tmp = service->endpoint;
	}
	SD_freeService(service);
*/
	string tmp;
	return tmp;
}
