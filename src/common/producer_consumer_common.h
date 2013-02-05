#pragma once

#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include "definitions.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

#define STALLED_DIR "/var/lib/fts3/stalled/"
#define MONITORING_DIR "/var/lib/fts3/monitoring/"
#define STATUS_DIR "/var/lib/fts3/status/"

int getDir (std::string dir, std::vector<std::string> &files);

void getUniqueTempFileName(const std::string& basename, std::string& tempname);
					   
void mktempfile(const std::string& basename, std::string& tempname);

//for monitoring
void runConsumerMonitoring(std::vector<std::string>& messages);

void runProducerMonitoring(const char* message);


//for receiving the status from url_copy
void runConsumerStatus(std::vector<struct message>& messages);

void runProducerStatus(struct message msg);


//for checking is the process is stalled
void runConsumerStall(std::vector<struct message_updater>& messages);

void runProducerStall(struct message_updater msg);
