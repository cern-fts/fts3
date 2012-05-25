#include "reporter.h"
#include <string>
#include <stdlib.h>

using namespace std;

Reporter::Reporter(){
	qm = new QueueManager(false);
	msg = new struct message;
}


Reporter::~Reporter(){
    if(msg)
	delete msg;    
    if (qm)
        delete qm;
}


void  Reporter::constructMessage(string job_id, string file_id, string transfer_status, string transfer_message){
	strcpy (msg->job_id, job_id.c_str());
	strcpy (msg->file_id, file_id.c_str());	
	strcpy (msg->transfer_status, transfer_status.c_str());	
	transfer_message = transfer_message.substr (0,1023);	
	strcpy (msg->transfer_message, transfer_message.c_str());
	qm->send(msg);	
}
