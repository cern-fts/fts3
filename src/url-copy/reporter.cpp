/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License. 
You may obtain a copy of the License at 

    http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and 
limitations under the License. */

#include "reporter.h"
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

using namespace std;

Reporter::Reporter() :source_se(""), dest_se("") , msg(NULL), qm(NULL), msg_updater(NULL),qm_updater(NULL){
  try{
    qm = new QueueManager(false);
    msg = new struct message;
   }catch(...){
        /*try again before let it fail*/
	     try{
		qm = new QueueManager(false);
		msg = new struct message;
	      }catch(...){
	   	/*no way to recover if an exception is thrown here, better let it fail and log the error*/	      
	      }
   }
   
  try{
    qm_updater = new QueueManager(true,false);
    msg_updater = new struct message_updater;
   }catch(...){
        /*try again before let it fail*/
	     try{
		qm_updater = new QueueManager(true, false);
		msg_updater = new struct message_updater;
	      }catch(...){
	   	/*no way to recover if an exception is thrown here, better let it fail and log the error*/	      
	      }
   }   
   
}

Reporter::~Reporter() {
    if (msg)
        delete msg;
    if (qm)
        delete qm;
    if (msg_updater)
        delete msg_updater;
    if (qm_updater)
        delete qm_updater;	
}

std::string Reporter::ReplaceNonPrintableCharacters(string s) {
    try {
        std::string result("");
        for (size_t i = 0; i < s.length(); i++) {
            char c = s[i];
            int AsciiValue = static_cast<int> (c);
            if (AsciiValue < 32 || AsciiValue > 127) {
                result.append(" ");
            } else {
                result += s.at(i);
            }
        }
        return result;
    } catch (...) {
        return s;
    }
}

void Reporter::constructMessage(string job_id, string file_id, string transfer_status, string transfer_message, double timeInSecs, double filesize) {
    try {
        strcpy(msg->job_id, job_id.c_str());
        strcpy(msg->file_id, file_id.c_str());
        strcpy(msg->transfer_status, transfer_status.c_str());
        transfer_message = transfer_message.substr(0, 1023);
        transfer_message = ReplaceNonPrintableCharacters(transfer_message);
        strcpy(msg->transfer_message, transfer_message.c_str());
        msg->process_id = (int) getpid();
        msg->timeInSecs = timeInSecs;
        msg->filesize = filesize;
        msg->nostreams = nostreams;
        msg->timeout = timeout;
        msg->buffersize = buffersize;
        strcpy(msg->source_se, source_se.c_str());
        strcpy(msg->dest_se, dest_se.c_str());
	if(qm){
        	qm->send(msg);
        }
    } catch (...) {
        //second attempt to resend the message
        strcpy(msg->job_id, job_id.c_str());
        strcpy(msg->file_id, file_id.c_str());
        strcpy(msg->transfer_status, transfer_status.c_str());
        transfer_message = transfer_message.substr(0, 1023);
        transfer_message = ReplaceNonPrintableCharacters(transfer_message);
        strcpy(msg->transfer_message, transfer_message.c_str());
        msg->process_id = (int) getpid();
        msg->timeInSecs = timeInSecs;
        msg->filesize = filesize;
        msg->nostreams = nostreams;
        msg->timeout = timeout;
        msg->buffersize = buffersize;
        strcpy(msg->source_se, source_se.c_str());
        strcpy(msg->dest_se, dest_se.c_str());
	if(qm)
        	qm->send(msg);

    }
}


void Reporter::constructMessageUpdater(std::string job_id, std::string file_id){
    try {
        strcpy(msg_updater->job_id, job_id.c_str());
        msg_updater->file_id = atoi(file_id.c_str());        
        msg_updater->process_id = (int) getpid();
	msg_updater->timestamp = std::time(NULL);
	if(qm_updater)
        	qm_updater->sendUpdater(msg_updater);
    } catch (...) {
        //attempt to resend the message
        strcpy(msg_updater->job_id, job_id.c_str());
        msg_updater->file_id = atoi(file_id.c_str());      
        msg_updater->process_id = (int) getpid();
	msg_updater->timestamp = std::time(NULL);
	if(qm_updater)
        	qm_updater->sendUpdater(msg_updater);
    }
}

