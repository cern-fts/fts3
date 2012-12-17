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
 * fts3-get-log.cpp
 *
 *  Created on: Dec 5, 2012
 *      Author: Michal Simon
 */

#include "GSoapContextAdapter.h"
#include "ui/JobIdCli.h"

#include <exception>
#include <iostream>
#include <vector>
#include <string>
#include <memory>


using namespace std;
using namespace fts3::cli;

/**
 * This is the entry point for the fts3-get-log command line tool.
 */
int main(int ac, char* av[]) {

	try {
		// create and initialize the command line utility
		auto_ptr<JobIdCli> cli (
				getCli<JobIdCli>(ac, av)
			);

		// validate command line options, and return respective gsoap context
		optional<GSoapContextAdapter&> opt = cli->validate();
		if (!opt.is_initialized()) return 0;
		GSoapContextAdapter& ctx = opt.get();

		vector<string> jobs = cli->getJobIds();
		vector<string>::iterator it;

		for (it = jobs.begin(); it != jobs.end(); it++) {
			string out = "/home/simonm/out.tar.gz";
			ctx.getLog(out, *it);
		}

		return 0;

    } catch(std::exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    } catch(string& ex) {
    	cout << ex << endl;
    	return 1;
    } catch(...) {
        cerr << "Exception of unknown type!\n";
        return 1;
    }

	return 0;
}

