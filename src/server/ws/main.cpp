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
 * main.cpp
 *
 *  Created on: Mar 8, 2012
 *      Author: Michal Simon
 */

#include "gsoap_stubs.h"
#include "db/generic/SingleDbInstance.h"
#include "config/serverconfig.h"

using namespace db;

int main(int ac, char* av[]) {

    static const int argc = 2;
    char *argv[argc] = {"executable", "--configfile=/etc/sysconfig/fts3config"};
	fts3::config::theServerConfig().read(argc, argv);

	DBSingleton::instance().getDBObjectInstance()->init(
			fts3::config::theServerConfig().get<string>("DbUserName"),
			fts3::config::theServerConfig().get<string>("DbPassword"),
			fts3::config::theServerConfig().get<string>("DbConnectString")
	);

	FileTransferSoapBindingService service;
	return service.run(8080);
}
