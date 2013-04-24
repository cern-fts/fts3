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
 * ConfigFileMonitor.cpp
 *
 *  Created on: Dec 6, 2012
 *      Author: simonm
 */

#include "FileMonitor.h"
#include "serverconfig.h"

#include "common/logger.h"

FTS3_CONFIG_NAMESPACE_START

using namespace fts3::common;

FileMonitor::FileMonitor(ServerConfig* sc) : sc(sc), running(false) {

}

FileMonitor::~FileMonitor() {

	// stop the monitoring thread
	if (monitor_thread.get()) {
		running = false;
		monitor_thread->interrupt();
	}

	// remove the filey from the watch list.
	inotify_rm_watch(fd, wd);

	// closing the INOTIFY instance
	close(fd);
}

void FileMonitor::start(string path) {
	// checkl if it's already running
	if (running) return;
	// set the running state
	running = true;

	file = getFile(path);

	// creating the INOTIFY instance
	fd = inotify_init();
	// checking for error
	if (fd < 0) {
		// todo
		//FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An error occurred during 'notify_init()': " << strerror(errno) << commit;
	}

	// add the file into watch list
	wd = inotify_add_watch(fd, getDir(path).c_str(), IN_MODIFY);


	// start monitoring thread
	monitor_thread.reset (
			new thread(run, this)
		);
}

void FileMonitor::stop() {
	// stop the monitoring thread
	running = false;
}

void FileMonitor::run (FileMonitor* const me) {

	char buffer[EVENT_BUF_LEN];
	// monitor the file
	while (me->running) {
		// read to determine the event change happens on the given file. Actually this read blocks until the change event occurs
		ssize_t length = read(me->fd, buffer, EVENT_BUF_LEN);

		// checking for error
		if (length < 0) {
			// todo
			// FTS3_COMMON_LOGGER_NEWLOG (INFO) << "An error occurred while monitoring the config file: " << strerror(errno)  << commit;
		}

		// actually read return the list of change events happens. Here, read the change event one by one and process it accordingly.
		for (int i = 0; i < length;) {
			inotify_event *event = (inotify_event*) &buffer[i];
			if (event->len) {
				if (me->file == event->name && (event->mask & IN_MODIFY)) {
					if (me->sc) {
						// todo it might be a good idea to save the option the program was start with at the beginning
						me->sc->read(0, 0);
					}
					break;
				}
			}
			i += EVENT_SIZE + event->len;
		}
	}
}

string FileMonitor::getDir(string path) {
	static const regex re("(.+)/[^/]+");
	smatch what;
	regex_match(path, what, re, match_extra);
	return what[1];
}

string FileMonitor::getFile(string path) {
	static const regex re(".+/([^/]+)");
	smatch what;
	regex_match(path, what, re, match_extra);
	return what[1];
}

FTS3_CONFIG_NAMESPACE_END
