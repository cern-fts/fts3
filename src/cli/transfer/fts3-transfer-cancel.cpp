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
 * fts3-config-get.cpp
 *
 *  Created on: Apr 3, 2012
 *      Author: Michał Simon
 */

#include "GSoapContextAdapter.h"
#include "MsgPrinter.h"
#include "ui/JobIdCli.h"

#include "common/JobStatusHandler.h"

#include <exception>
#include <iostream>
#include <vector>
#include <string>

#include <boost/scoped_ptr.hpp>

using namespace boost;
using namespace std;
using namespace fts3::cli;
using namespace fts3::common;

/**
 * This is the entry point for the fts3-transfer-cancel command line tool.
 */
int main(int ac, char* av[]) {

	scoped_ptr<JobIdCli> cli;

	try {
		// create and initialize the command line utility
		cli.reset(
				getCli<JobIdCli>(ac, av)
			);

		// validate command line options, and return respective gsoap context
		optional<GSoapContextAdapter&> opt = cli->validate();
		if (!opt.is_initialized()) return 0;
		GSoapContextAdapter& ctx = opt.get();

		vector<string> jobs = cli->getJobIds();

		if (jobs.empty()) {
			cli->printer().missing_parameter("Job ID");
			return 0;
		}

		ctx.cancel(jobs);

		vector<string>::iterator it;
	    for (it = jobs.begin(); it < jobs.end(); it++) {
	    	cli->printer().cancelled_job(*it);

	    }

    } catch(std::exception& ex) {
    	if (cli.get())
    		cli->printer().error_msg(ex.what());
        return 1;
    } catch(string& ex) {
    	if (cli.get())
    		cli->printer().gsoap_error_msg(ex);
    	return 1;
    } catch(...) {
    	if (cli.get())
    		cli->printer().error_msg("Exception of unknown type!");
        return 1;
    }

	return 0;
}
