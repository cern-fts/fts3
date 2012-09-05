#include "reporter.h"
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

using namespace std;

Reporter::Reporter(): source_se(""), dest_se(""){
	qm = new QueueManager(false);
	msg = new struct message;
}


Reporter::~Reporter(){
    if(msg)
	delete msg;    
    if (qm)
        delete qm;
}


void  Reporter::constructMessage(string job_id, string file_id, string transfer_status, string transfer_message, double timeInSecs,  double filesize){
	try{
	strcpy (msg->job_id, job_id.c_str());
	strcpy (msg->file_id, file_id.c_str());	
	strcpy (msg->transfer_status, transfer_status.c_str());	
	transfer_message = transfer_message.substr (0,1023);	
	strcpy (msg->transfer_message, transfer_message.c_str());
	msg->process_id = (int) getpid();
	msg->timeInSecs = timeInSecs;
	msg->filesize = filesize;
	msg->nostreams = nostreams;
        msg->timeout = timeout;
        msg->buffersize = buffersize;		
	strcpy(msg->source_se, source_se.c_str()); 
	strcpy(msg->dest_se, dest_se.c_str()); 
	qm->send(msg);	
	}catch(...){
	//attempt to resend the message
	strcpy (msg->job_id, job_id.c_str());
	strcpy (msg->file_id, file_id.c_str());	
	strcpy (msg->transfer_status, transfer_status.c_str());	
	transfer_message = transfer_message.substr (0,1023);	
	strcpy (msg->transfer_message, transfer_message.c_str());
	msg->process_id = (int) getpid();
	msg->timeInSecs = timeInSecs;
	msg->filesize = filesize;
	msg->nostreams = nostreams;
        msg->timeout = timeout;
        msg->buffersize = buffersize;		
	strcpy(msg->source_se, source_se.c_str()); 
	strcpy(msg->dest_se, dest_se.c_str()); 
	qm->send(msg);		
	
	}
}
