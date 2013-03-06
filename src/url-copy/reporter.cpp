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
#include "boost/date_time/gregorian/gregorian.hpp"
#include <boost/tokenizer.hpp>
#include "producer_consumer_common.h"
#include <algorithm>
#include <ctime>

using namespace std;

Reporter::Reporter() :source_se(""), dest_se("") , msg(NULL), msg_updater(NULL) {
	msg = new struct message();	
	msg_updater = new struct message_updater();	
}

Reporter::~Reporter() {
    if (msg)
        delete msg;
    if (msg_updater)
        delete msg_updater;	
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

void Reporter::constructMessage(bool retry, string job_id, string file_id, string transfer_status, string transfer_message, double timeInSecs, double filesize) {
    try {
        strcpy(msg->job_id, job_id.c_str());
        msg->file_id  = boost::lexical_cast<unsigned int>(file_id);
        strcpy(msg->transfer_status, transfer_status.c_str());
	if(transfer_message.length() > 0 && transfer_message.length() >= 1023){
        	transfer_message = transfer_message.substr(0, 1023);
        	transfer_message = ReplaceNonPrintableCharacters(transfer_message);
        	strcpy(msg->transfer_message, transfer_message.c_str());
	}else if(transfer_message.length() > 0 && transfer_message.length() < 1023){
        	transfer_message = ReplaceNonPrintableCharacters(transfer_message);
        	strcpy(msg->transfer_message, transfer_message.c_str());	
	}else{		
		memset(msg->transfer_message, 0, sizeof (msg->transfer_message));
	}
	
        msg->process_id = (int) getpid();
        msg->timeInSecs = timeInSecs;
        msg->filesize = filesize;
        msg->nostreams = nostreams;
        msg->timeout = timeout;
        msg->buffersize = buffersize;
        strcpy(msg->source_se, source_se.c_str());
        strcpy(msg->dest_se, dest_se.c_str());
	msg->timestamp = milliseconds_since_epoch();
	msg->retry = retry;
	runProducerStatus(*msg);      
    } catch (...) {
        //second attempt to resend the message
        strcpy(msg->job_id, job_id.c_str());
        msg->file_id  = boost::lexical_cast<unsigned int>(file_id);
        strcpy(msg->transfer_status, transfer_status.c_str());
	if(transfer_message.length() > 0 && transfer_message.length() >= 1023){
        	transfer_message = transfer_message.substr(0, 1023);
        	transfer_message = ReplaceNonPrintableCharacters(transfer_message);
        	strcpy(msg->transfer_message, transfer_message.c_str());
	}else if(transfer_message.length() > 0 && transfer_message.length() < 1023){
        	transfer_message = ReplaceNonPrintableCharacters(transfer_message);
        	strcpy(msg->transfer_message, transfer_message.c_str());	
	}else{		
		memset(msg->transfer_message, 0, sizeof (msg->transfer_message));
	}
	
        msg->process_id = (int) getpid();
        msg->timeInSecs = timeInSecs;
        msg->filesize = filesize;
        msg->nostreams = nostreams;
        msg->timeout = timeout;
        msg->buffersize = buffersize;
        strcpy(msg->source_se, source_se.c_str());
        strcpy(msg->dest_se, dest_se.c_str());
	msg->timestamp = milliseconds_since_epoch();
	msg->retry = retry;	
	runProducerStatus(*msg);        
    }
}


void Reporter::constructMessageUpdater(std::string job_id, std::string file_id){
    try {
        strcpy(msg_updater->job_id, job_id.c_str());
        msg_updater->file_id = boost::lexical_cast<unsigned int>(file_id);   
        msg_updater->process_id = (int) getpid();
	msg_updater->timestamp = milliseconds_since_epoch();
	runProducerStall(*msg_updater);
    } catch (...) {
        //attempt to resend the message
        strcpy(msg_updater->job_id, job_id.c_str());
        msg_updater->file_id = boost::lexical_cast<unsigned int>(file_id);      
        msg_updater->process_id = (int) getpid();
	msg_updater->timestamp = milliseconds_since_epoch();
	runProducerStall(*msg_updater);
    }
}

