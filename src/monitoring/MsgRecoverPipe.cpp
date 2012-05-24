/**
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://www.eu-egee.org/partners/ for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  
 * 
 */


#include "MsgRecoverPipe.h"
#include "half_duplex.h" /* For name of the named-pipe */
#include "utility_routines.h"
#include <iostream>
#include <string>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "concurrent_queue.h"
#include "Logger.h"
#include "compress.h"
#include <vector>


/*
This "recover" thread is used to temporarly store msgs if broker is down is connectionlost for some reason
The named pipe is created & opened open when the thread starts and detroyed when thread yields
*/


std::vector<std::string> testRecoverStr(std::string & s, std::string & lastref) {

    std::vector<std::string> elems;
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = s.find('\4', pos)) != std::string::npos) {
        std::string substring(s.substr(prev_pos, pos - prev_pos));

        elems.push_back(substring);
        prev_pos = ++pos;
    }
    lastref = s.substr(prev_pos, pos - prev_pos); // Last word
    return elems;
}

MsgRecoverPipe::MsgRecoverPipe(char * pipeName) {

    umask(0);
    pName = pipeName;
}

MsgRecoverPipe::~MsgRecoverPipe() {
    cleanup();
}

int MsgRecoverPipe::isready(int fd) {
    int rc = 0;
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = tv.tv_usec = 0;

    rc = select(fd + 1, &fds, NULL, NULL, &tv);
    if (rc < 0)
        return -1;

    return FD_ISSET(fd, &fds) ? 1 : 0;
}

void MsgRecoverPipe::sendRecoveredMessages(){
	if(concurrent_queue::getInstance()->size() > 0){
        while(concurrent_queue::getInstance()->empty()){
		if(concurrent_queue::getInstance()->size() == 0)
			break;
		std::string message = concurrent_queue::getInstance()->pop();
		bool sent = send_message(message);	
                if(sent == false)
			concurrent_queue::getInstance()->push(message);
        }
	} 
}

void MsgRecoverPipe::run() {
    std::string msgstring;
    fpout = fdopen(fd, "r");
    register int ret = 0;
    char buffer[4096] = {0};
    int res = 0;
    std::string msg("");
    std::string lastref("");
    std::vector<std::string>::iterator i;
    
    umask(0);
    struct stat st;
    int result_code = -1;
    if (stat("/var/spool/ftsmsg", &st) != 0) {
        result_code = mkdir("/var/spool/ftsmsg", 0777);
        if (result_code != 0) {
            logger::writeLog("Cannot create dir /var/spool/ftsmsg: " + std::string(strerror(errno)), true);
            exit(0);
        }
    }

    /* Create the named - pipe */
    if (access(pName, F_OK) == -1) {
        umask(0);
        ret_val = mkfifo(pName, 0777);

        if ((ret_val != 0) && (errno != EEXIST)) {
            logger::writeLog("Cannot creating named pipe: " + std::string(strerror(errno)), true);
        }
    }
    /* Open the pipe for reading/writing(writing is required for blocking) */
    fd = open(pName, O_RDWR);
    if (fd < 0) {
        logger::writeLog("Cannot opening named pipe: " + std::string(strerror(errno)), true);
    }
    
    
    while (1) {
        ret = isready(fd);
        res = read(fd, buffer, 4096);
	if(res != -1)
        {
        	buffer[res] = 0;

        	msg += buffer;

        	std::vector<std::string> myVect = testRecoverStr(msg, lastref);
        	for (i = myVect.begin();
                	i != myVect.end();
                	++i) { 			        
            		concurrent_queue::getInstance()->push(*i);					
        	}
	
        	msg = lastref;
        	lastref.clear();
	}
    }
}

void MsgRecoverPipe::cleanup() {
    if (fd != -1)
        close(fd);
}
