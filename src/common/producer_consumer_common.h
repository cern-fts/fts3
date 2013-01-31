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
