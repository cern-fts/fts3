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
 *
 * ConfigFileMonitor.h
 *
 *  Created on: Dec 6, 2012
 *      Author: simonm
 */

#ifndef FILEMONITOR_H_
#define FILEMONITOR_H_

#include "config_dev.h"

#include <sys/inotify.h>
#include <string>

#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>

FTS3_CONFIG_NAMESPACE_START

using namespace std;
using namespace boost;

class ServerConfig;

class FileMonitor {

public:
	FileMonitor(ServerConfig* sc);
	virtual ~FileMonitor();

	void start(string path);
	void stop();

	static void run (FileMonitor* const me);

private:

	string getDir(string path);
	string getFile(string path);

	ServerConfig* sc;

	string file;
	bool running;

	int fd;
	int wd;

	scoped_ptr<thread> monitor_thread;

	static const int EVENT_SIZE = sizeof (inotify_event);
	static const int EVENT_BUF_LEN = 1024 * (EVENT_SIZE + 16);
};

FTS3_CONFIG_NAMESPACE_END
#endif /* CONFIGFILEMONITOR_H_ */
