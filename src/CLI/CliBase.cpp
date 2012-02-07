/*
 * CliBase.cpp
 *
 *  Created on: Feb 2, 2012
 *      Author: simonm
 */

#include "CliBase.h"
#include  "CliManager.h"

#include <iostream>

#include "ServiceDiscoveryIfce.h" // check "" or <>
#include <glite/data/glite-util.h>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>

using namespace boost;

CliBase::CliBase() : basic("Basic options") {
    basic.add_options()
			("help,h", "Print this help text and exit.")
			("quite,q", "Quiet operation.")
			("verbose,v", "Be more verbose.")
			("service,s", value<string>(), "Use the transfer service at the specified URL.")
			("source", value<string>(), "Specify source site name.")
			("destination", value<string>(), "Specify destination site name.")
			("version,V", "Print the version number and exit.")
			;

    cli_options.add(basic);

	p.add("source", 1);
	p.add("destination", 1);
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
		endpoint = discoverService();
	}
}

bool CliBase::printHelp() {

	if (vm.count("help")) {
        cout << cli_options << endl;
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

bool CliBase::isVerbose() {
	return vm.count("verbose");
}

bool CliBase::isQuite() {
	return vm.count("quite");
}

string CliBase::getService() {

	return endpoint;
}

string CliBase::getSource() {

	if (vm.count("source")) {
		return vm["source"].as<string>();
	}
	return "";
}

string CliBase::getDestination() {

	if (vm.count("destination")) {
		return vm["destination"].as<string>();
	}
	return "";
}

string CliBase::discoverService() {

	CliManager* manager = CliManager::getInstance();

	string source = getSource(), destination = getDestination();
	SDService *service;
	scoped_array<char> c_str_arr, error_arr;
	char *error, *c_str = 0;
	const char * fts_srvtype;
	const char * fts_version;
	SDException exc;

	if (source.size() > 0 && destination.size() > 0) {

		SDServiceData datas[] =
		{
			{"source", (char*) source.c_str()},
			{"destination", (char*) destination.c_str()}
		};

		SDServiceDataList datalist = {NULL, 2, datas};

		c_str = glite_discover_service_by_data(manager->FTS3_CA_SD_TYPE.c_str(), &datalist, &error);
		c_str_arr.reset(c_str);
		error_arr.reset(error);
	}

	if (getenv(manager->FTS3_SD_ENV.c_str()))
		fts_srvtype = getenv(manager->FTS3_SD_ENV.c_str());
	else
		fts_srvtype = manager->FTS3_SD_TYPE.c_str();

	if(getenv(manager->FTS3_IFC_VERSION.c_str()))
		fts_version = getenv(manager->FTS3_IFC_VERSION.c_str());
	else
		fts_version = manager->FTS3_INTERFACE_VERSION.c_str();

	// Note: glite_discover_service will check if the service name passed
	// has an associated service of the given type (fts_srvtype), if not
	// it will check if the name refers to a site name or an host name

	c_str = glite_discover_service_by_version(fts_srvtype, c_str, fts_version, &error);
	c_str_arr.reset(c_str);
	error_arr.reset(error);

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

	return tmp;
}
