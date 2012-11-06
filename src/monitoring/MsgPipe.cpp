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


#include "MsgPipe.h"
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

void handler(int sig) {
    sig = 0;
    exit(0);
}

MsgPipe::MsgPipe(std::string qname) {
     try {
        qm = new QueueManager(true, qname, true);        
    } catch (interprocess_exception &ex) {
        /*shared mem segment already exists, reuse it*/
	if(qm)
		delete qm;       
	try{
		qm = new QueueManager(false, qname, true);                 
	}
	catch (interprocess_exception &ex) {
		FTS3_COMMON_EXCEPTION_THROW(Err_Custom(ex.what()));
	}	
    }catch(...){
        if(qm)
                delete qm;
	qm = new QueueManager(false, qname, true);
    }    
    
    //register sig handler to cleanup resources upon exiting
    signal(SIGFPE, handler);
    signal(SIGILL, handler);
    signal(SIGSEGV, handler);
    signal(SIGBUS, handler);
    signal(SIGABRT, handler);
    signal(SIGTERM, handler);
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);

}

MsgPipe::~MsgPipe() {
    cleanup();
}


void MsgPipe::run() {
    char tmpMsg[5120]={0};
    while (1){
     try{
   	qm->msg_receive(tmpMsg);
	concurrent_queue::getInstance()->push(std::string(tmpMsg));					    
      }
     catch (interprocess_exception &ex) {
                FTS3_COMMON_EXCEPTION_THROW(Err_Custom(ex.what()));
        }
     catch (...) {
                FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Message queue thrown exception"));
        }
    }
}

void MsgPipe::cleanup() {

}
