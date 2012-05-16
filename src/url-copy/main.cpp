#include <iostream>
#include <ctype.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <transfer/gfal_transfer.h>
#include "common/logger.h"
#include "common/error.h"
#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include "mq_manager.h"
#include <fstream>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "parse_url.h"
#include <uuid/uuid.h>
#include <vector>


using namespace FTS3_COMMON_NAMESPACE;
using namespace std;
namespace po = boost::program_options;


void archive(std::string &filename){  
}


void constructMessage(QueueManager* qm, struct message* msg, string job_id, string file_id, string transfer_status, string transfer_message){
	strcpy (msg->job_id, job_id.c_str());
	strcpy (msg->file_id, file_id.c_str());	
	strcpy (msg->transfer_status, transfer_status.c_str());		
	strcpy (msg->transfer_message, transfer_message.c_str());
	qm->send(msg);			
}


std::string generateLogFileName(std::string surl, std::string durl, std::string & file_id){
   std::string new_name = std::string("");
   char *base_scheme = NULL;
   char *base_host = NULL;
   char *base_path = NULL;
   int base_port = 0;
   std::string shost("");
   std::string dhost(""); 

    //add surl / durl
    //source
    parse_url(surl.c_str(), &base_scheme, &base_host, &base_port, &base_path);
    shost = base_host;
    
    //dest
    parse_url(durl.c_str(), &base_scheme, &base_host, &base_port, &base_path); 
    dhost = base_host;

    // add date
    time_t current;
    time(&current);
    struct tm * date = gmtime(&current);

    // Create template
    std::stringstream ss;
    ss << std::setfill('0');
    ss << std::setw(4) << (date->tm_year+1900)
       <<  "-" << std::setw(2) << (date->tm_mon+1)
       <<  "-" << std::setw(2) << (date->tm_mday)
       <<  "-" << std::setw(2) << (date->tm_hour)
               << std::setw(2) << (date->tm_min)
    << "__" << shost << "__" << dhost << "__" << file_id;

    new_name += ss.str();
    return new_name;
}

class logger {
public:

    logger( std::ostream& os_ );
     ~logger() {os << std::endl;}

    template<class T>
    friend logger& operator<<( logger& log, const T& output );

private:

    std::ostream& os;
};

logger::logger( std::ostream& os_) : os(os_) {}

template<class T>
logger& operator<<( logger& log, const T& output ) {
    log.os << output;
    log.os.flush(); 
    return log;
}

int main(int argc, char **argv) {

    std::string file_id("");
    std::string job_id("");   //a
    std::string source_url(""); //b
    std::string dest_url(""); //c
    bool overwrite = false; //d
    int nbstreams = 5;     //e
    int tcpbuffersize = 0; //f
    int blocksize = 0; //g
    int timeout = 3600; //h
    bool daemonize = true; //i
    std::string dest_token_desc(""); //j
    std::string source_token_desc("");  //k
    int markers_timeout = 180;  //l
    int first_marker_timeout = 180; //m
    int srm_get_timeout = 180; //n
    int srm_put_timeout = 180; //o	
    int http_timeout = 180; //p
    bool dont_ping_source = false;//q
    bool dont_ping_dest = false; //r
    bool disable_dir_check = false; //s
    int copy_pin_lifetime = 0; //t
    bool lan_connection = false; //u
    bool fail_nearline = false; //v
    int timeout_per_mb = 0; //w
    int no_progress_timeout = 180; //x
    std::string algorithm("");//y
    std::string checksum_value("");//z
    bool compare_checksum = true; //A
    
    struct message* msg = new struct message;
    
  
    for(int i(1); i < argc; ++i){             	
       std::string temp(argv[i]);
        if(temp.compare("-y") == 0)
	 algorithm = std::string(argv[i+1]);		 
        if(temp.compare("-z") == 0)
	 checksum_value = std::string(argv[i+1]);	
        if(temp.compare("-A") == 0)
	 compare_checksum = true;	 	 	        
        if(temp.compare("-w") == 0)
	 timeout_per_mb = std::atoi(argv[i+1]);		 	  
        if(temp.compare("-x") == 0)
	 no_progress_timeout = std::atoi(argv[i+1]);	       
        if(temp.compare("-u") == 0)
	 lan_connection = true;	       
        if(temp.compare("-v") == 0)
	 fail_nearline = true;	       
        if(temp.compare("-t") == 0)
	 copy_pin_lifetime = std::atoi(argv[i+1]);       
        if(temp.compare("-q") == 0)
	 dont_ping_source = true;	       
        if(temp.compare("-r") == 0)
	 dont_ping_dest = true;	
        if(temp.compare("-s") == 0)
	 disable_dir_check = true;		 	        
        if(temp.compare("-a") == 0)
	 job_id = std::string(argv[i+1]);
        if(temp.compare("-b") == 0)
	 source_url = std::string(argv[i+1]);	 
        if(temp.compare("-c") == 0)
	 dest_url = std::string(argv[i+1]);
        if(temp.compare("-d") == 0)
	 overwrite = true;	 	 	 
        if(temp.compare("-e") == 0)
	 nbstreams = std::atoi(argv[i+1]);		 
        if(temp.compare("-f") == 0)
	 tcpbuffersize = std::atoi(argv[i+1]);		 	 
        if(temp.compare("-g") == 0)
	 blocksize = std::atoi(argv[i+1]);		 	 	 
        if(temp.compare("-h") == 0)
	 timeout = std::atoi(argv[i+1]);		 	 	 	 
        if(temp.compare("-i") == 0)
	 daemonize = true;		 	 	 	 
        if(temp.compare("-j") == 0)
	 dest_token_desc = std::string(argv[i+1]);		 
        if(temp.compare("-k") == 0)
	 source_token_desc = std::string(argv[i+1]);			 
        if(temp.compare("-l") == 0)
	 markers_timeout = std::atoi(argv[i+1]);		 	  
        if(temp.compare("-m") == 0)
	 first_marker_timeout = std::atoi(argv[i+1]);		 	  
        if(temp.compare("-n") == 0)
	 srm_get_timeout = std::atoi(argv[i+1]);		 	  
        if(temp.compare("-o") == 0)
	 srm_put_timeout = std::atoi(argv[i+1]);		 	  
        if(temp.compare("-p") == 0)
	 http_timeout = std::atoi(argv[i+1]);		 	  	 	 	 	 
        if(temp.compare("-B") == 0)
	 file_id = std::string(argv[i+1]);		 	  	 	 	 	 	 
    }
      
    std::string logFileName = "/var/log/fts3/";
    logFileName += generateLogFileName(source_url, dest_url, file_id);
    std::ofstream logStream;
    logStream.open (logFileName.c_str(), ios::app);    
    logger log(logStream);    

    log << "Transfer accepted"<< '\n';
    log << "job_id:" << job_id<< '\n'; //a
    log << "file_id:" << file_id<< '\n'; //a
    log << "source_url:" << source_url<< '\n'; //b
    log << "dest_url:" << dest_url<< '\n'; //c
    log << "overwrite:" << overwrite<< '\n'; //d
    log << "nbstreams:" << nbstreams<< '\n';     //e
    log << "tcpbuffersize:" << tcpbuffersize<< '\n'; //f
    log << "blocksize:" << blocksize<< '\n'; //g
    log << "timeout:" << timeout<< '\n'; //h
    log << "daemonize:" << daemonize<< '\n'; //i
    log << "dest_token_desc:" << dest_token_desc<< '\n'; //j
    log << "source_token_desc:" << source_token_desc<< '\n';  //k
    log << "markers_timeout:" << markers_timeout<< '\n';  //l
    log << "first_marker_timeout:" << first_marker_timeout; //m
    log << "srm_get_timeout:" << srm_get_timeout<< '\n'; //n
    log << "srm_put_timeout:" << srm_put_timeout<< '\n'; //o	
    log << "http_timeout:" << http_timeout<< '\n'; //p
    log << "dont_ping_source:" << dont_ping_source<< '\n';//q
    log << "dont_ping_dest:" << dont_ping_dest<< '\n'; //r
    log << "disable_dir_check:" << disable_dir_check<< '\n'; //s
    log << "copy_pin_lifetime:" << copy_pin_lifetime<< '\n'; //t
    log << "lan_connection:" << lan_connection<< '\n'; //u
    log << "fail_nearline:" << fail_nearline<< '\n'; //v
    log << "timeout_per_mb:" << timeout_per_mb<< '\n'; //w
    log << "no_progress_timeout:" << no_progress_timeout<< '\n'; //x
    log << "algorithm:" << algorithm<< '\n';//y
    log << "checksum_value:" << checksum_value<< '\n';//z
    log << "compare_checksum:" << compare_checksum<< '\n'; //A
    

    log << "Setup the fts3_server reporter queue"<< '\n';
    //init message_queue
    QueueManager* qm = NULL;
    try {
        qm = new QueueManager(false);
    } catch (interprocess_exception &ex) {
        if (qm)
            delete qm;
        log << ex.what()<< '\n';
    }
    
    
    constructMessage(qm, msg, job_id, file_id, "STARTED", "Transfer started");
    

    //start transfer gfal2	
    GError * tmp_err = NULL; // classical GError/glib error management
    gfal_context_t handle;
    int ret = -1;
  
    log << "initialize gfal2"<< '\n';   
    if ((handle = gfal_context_new(&tmp_err)) == NULL) {
	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Transfer Initialization failed - errno: " <<  tmp_err->message << " Error message:" << tmp_err->message << commit;
	log << "Transfer Initialization failed - errno: " <<  tmp_err->message << " Error message:" << tmp_err->message<< '\n';
	constructMessage(qm, msg, job_id, file_id, "FINISHED", std::string(tmp_err->message));
    }
    log << "begin copy"<< '\n';
        FTS3_COMMON_LOGGER_NEWLOG (INFO) <<  " Transfer Started" << commit;
    if ((ret = gfalt_copy_file(handle, NULL, source_url.c_str(), dest_url.c_str(), &tmp_err)) != 0) {        
	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Transfer failed - errno: " <<  tmp_err->message << " Error message:" << tmp_err->message << commit;
	log << "Transfer failed - errno: " <<  tmp_err->message << " Error message:" << tmp_err->message<< '\n';
	constructMessage(qm, msg, job_id, file_id, "FINISHED", std::string(tmp_err->message));	
    } else{
        FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Transfer completed successfully"  << commit;
	log << "Transfer completed successfully"<< '\n';
		constructMessage(qm, msg, job_id, file_id, "FINISHED", "");
    }
    
    log << "release all gfal2 resources"<< '\n';
    gfal_context_free(handle);
    if(msg)
	delete msg;    
    if (qm)
        delete qm;

    archive(logFileName);
    return EXIT_SUCCESS;
}

