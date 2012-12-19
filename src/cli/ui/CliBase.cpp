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
 * CliBase.cpp
 *
 *  Created on: Feb 2, 2012
 *      Author: Michal Simon
 */

#include "CliBase.h"

#include <iostream>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace fts3::cli;

const string CliBase::error = "error";
const string CliBase::result = "result";
const string CliBase::parameter_error = "parameter_error";

CliBase::CliBase(): visible("Allowed options"), ctx(0) {

	// add basic command line options
    basic.add_options()
			("help,h", "Print this help text and exit.")
			("quite,q", "Quiet operation.")
			("verbose,v", "Be more verbose.")
			("service,s", value<string>(), "Use the transfer service at the specified URL.")
			("version,V", "Print the version number and exit.")
			("json,j", "The output should be printed in JSON format")
			;

    version = getCliVersion();
    interface = version;
}

CliBase::~CliBase() {
	if (ctx) {
		delete ctx;
	}
}

void CliBase::parse(int ac, char* av[]) {

	toolname = av[0];

	// add specific and hidden parameters to all parameters
	all.add(basic).add(specific).add(hidden);
	// add specific parameters to visible parameters (printed in help)
	visible.add(basic).add(specific);

	// turn off guessing, so --source is not mistaken with --source-token
	int style = command_line_style::default_style & ~command_line_style::allow_guessing;

	// parse the options that have been used
	store(command_line_parser(ac, av).options(all).positional(p).style(style).run(), vm);
	notify(vm);

	// check is the output is verbose
	msgPrinter.setVerbose(
			vm.count("verbose")
		);
	// check if the output is in json format
	msgPrinter.setJson(
			vm.count("json")
		);

	// check whether the -s option has been used
	if (vm.count("service")) {
		endpoint = vm["service"].as<string>();
		// check if the endpoint has the right prefix
		if (endpoint.find("http") != 0 && endpoint.find("https") != 0 && endpoint.find("httpd") != 0) {
			msgPrinter.wrong_endpoint_format(endpoint);
			// if not erase
			endpoint.erase();
		}
	} else {

		if (useSrvConfig()) {
			fstream  cfg ("/etc/fts3/fts3config");
			if (cfg.is_open()) {

				string ip;
				string port;
				string line;

				do {
					getline(cfg, line);
					if (line.find("#") != 0 && line.find("Port") != string::npos) {
						// erase 'Port='
						port = line.erase(0, 5);

					} else if (line.find("#") != 0 && line.find("IP") != string::npos) {
						// erase 'IP='
						ip += line.erase(0, 3);
					}

				} while(!cfg.eof());

				if (!ip.empty() && !port.empty())
					endpoint = "https://" + ip + ":" + port;
			}
		} else if (!vm.count("help") && !vm.count("version")) {
			// if the -s option has not been used try to discover the endpoint
			// (but only if -h and -v option were not used)
			endpoint = discoverService();
		}
	}
}

optional<GSoapContextAdapter&> CliBase::validate(bool init) {

	// if applicable print help or version and exit, nothing else needs to be done
	if (printHelp(toolname) || printVersion()) return optional<GSoapContextAdapter&>();

	// if endpoint could not be determined, we cannot do anything
	if (endpoint.empty()) {
		throw string("Failed to determine the endpoint");
	}

	// create and initialize gsoap context
	ctx = new GSoapContextAdapter(endpoint);
	if (init) ctx->init();

	// if verbose print general info
	if (isVerbose()) {
		msgPrinter.endpoint(ctx->getEndpoint());
		msgPrinter.service_version(ctx->getVersion());
		msgPrinter.service_interface(ctx->getInterface());
		msgPrinter.service_schema(ctx->getSchema());
		msgPrinter.service_metadata(ctx->getMetadata());
		msgPrinter.client_version(version);
		msgPrinter.client_interface(interface);
	}

	return *ctx;
}

string CliBase::getUsageString(string tool) {
	return "Usage: " + tool + " [options]";
}

bool CliBase::printHelp(string tool) {

	// check whether the -h option was used
	if (vm.count("help")) {

		// remove the path to the executive
		size_t pos = tool.find_last_of('/');
		if( pos != string::npos) {
			tool = tool.substr(pos + 1);
		}
		// print the usage guigelines
		cout << endl << getUsageString(tool) << endl << endl;
		// print the available options
        cout << visible << endl;
        return true;
    }

    return false;
}

bool CliBase::printVersion() {

	// check whether the -V option was used
	if (vm.count("version")) {
		msgPrinter.version(version);
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

//void CliBase::print (string name, string msg, bool verbose_only) {
//	// if verbose flag is required and CLI has been muted return
//	// if json output is used return
//	if ( (verbose_only && !isverbose)) return;
//
//	if (isjson) {
//		json_out.add(name, msg);
//	} else {
//		cout << name << " : " << msg << endl;
//	}
//}

string CliBase::getCliVersion() {
    FILE *in;
    char buff[512];

    in = popen("rpm -q --qf '%{VERSION}' fts-client", "r");

    stringstream ss;
    while (fgets(buff, sizeof (buff), in) != NULL) {
        ss << buff;
    }

    pclose(in);

    return "0.0.1";
    return ss.str();
}
