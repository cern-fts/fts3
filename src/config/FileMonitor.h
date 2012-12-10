/*
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
