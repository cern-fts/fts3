#include <iostream>
#include <ctype.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <stdio.h>
#include <pwd.h>
#include <transfer/gfal_transfer.h>
#include "common/logger.h"
#include "common/error.h"
#include "mq_manager.h"
#include <fstream>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "parse_url.h"
#include <uuid/uuid.h>
#include <vector>
#include <gfal_api.h>
#include <memory>
#include "file_management.h"
#include "reporter.h"
#include "logger.h"
#include "msg-ifce.h"
#include "errors.h"
#include "signal_logger.h"
#include "UserProxyEnv.h"
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>


using namespace FTS3_COMMON_NAMESPACE;
using namespace std;

static FileManagement fileManagement;
static Reporter reporter;
static std::ofstream logStream;
static transfer_completed tr_completed;
static std::string g_file_id("");
static std::string g_job_id(""); 
static std::string errorScope("");
static std::string errorPhase("");
static std::string reasonClass("");
static std::string errorMessage("");

static int   orig_ngroups = -1;
static gid_t orig_gid = -1;
static uid_t orig_uid = -1;
static gid_t orig_groups[NGROUPS_MAX];

static std::vector<std::string> split(const char *str, char c = ':')
{
    std::vector<std::string> result;

    while(1)
    {
        const char *begin = str;

        while(*str != c && *str)
                str++;

        result.push_back(string(begin, str));

        if(0 == *str++)
                break;
    }

    return result;
}



void call_perf(gfalt_transfer_status_t h, const char* src, const char* dst, gpointer user_data){

	size_t avg  =  gfalt_copy_get_average_baudrate(h,NULL) / 1024;  
	size_t inst =  gfalt_copy_get_instant_baudrate(h,NULL) / 1024; 
	size_t trans=  gfalt_copy_get_bytes_transfered(h,NULL);
	time_t elapsed  = gfalt_copy_get_elapsed_time(h,NULL);
	   
	logStream << fileManagement.timestamp() <<  "INFO bytes:" << trans << ", avg KB/sec :" << avg << ", inst KB/sec :" << inst << ", elapsed:" << elapsed << '\n';
        FTS3_COMMON_LOGGER_NEWLOG(INFO) <<  "INFO bytes:" << trans << ", avg KB/sec :" << avg << ", inst KB/sec:" << inst << ", elapsed:" << elapsed <<  commit;
}



void signalHandler( int signum )
{   
    logStream << fileManagement.timestamp() <<  "WARN Interrupt signal received" << '\n';
    msg_ifce::getInstance()->set_transfer_error_scope(&tr_completed, errorScope);
    msg_ifce::getInstance()->set_transfer_error_category(&tr_completed, reasonClass);
    msg_ifce::getInstance()->set_failure_phase(&tr_completed, errorPhase);
    msg_ifce::getInstance()->set_transfer_error_message(&tr_completed, "Transfer canceled by the user");
    msg_ifce::getInstance()->set_final_transfer_state(&tr_completed, "Abort");    
    msg_ifce::getInstance()->set_tr_timestamp_complete(&tr_completed, msg_ifce::getInstance()->getTimestamp());
    msg_ifce::getInstance()->SendTransferFinishMessage(&tr_completed);
    reporter.constructMessage(g_job_id, g_file_id, "CANCELED", "Transfer canceled by the user");
    logStream.close();
    fileManagement.archive();    
    exit(signum);  
}


/*courtesy of:
"Setuid Demystified" by Hao Chen, David Wagner, and Drew Dean: http://www.cs.berkeley.edu/~daw/papers/setuid-usenix02.pdf
*/
uid_t name_to_uid(char const *name)
{
  if (!name)
    return -1;
  long const buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (buflen == -1)
    return -1;
  // requires c99
  char buf[buflen];
  struct passwd pwbuf, *pwbufp;
  if (0 != getpwnam_r(name, &pwbuf, buf, buflen, &pwbufp)
      || !pwbufp)
    return -1;
  return pwbufp->pw_uid;
}

int main(int argc, char **argv) {

    //switch to non-priviledged user to avoid reading the hostcert
    uid_t privid = geteuid();     
    char user[ ]  ="fts3";      
    uid_t pw_uid;
    pw_uid = name_to_uid(user);

 
    REGISTER_SIGNAL(SIGABRT);
    REGISTER_SIGNAL(SIGSEGV);
    REGISTER_SIGNAL(SIGTERM);
        
    std::string bytes_to_string("");
    //transfer_completed tr_completed;
    struct stat statbufsrc;
    struct stat statbufdest;
    long double source_size = 0;
    long double dest_size = 0;
    GError * tmp_err = NULL; // classical GError/glib error management   
    gfalt_params_t params = gfalt_params_handle_new(&tmp_err);
    gfal_context_t handle;
    int ret = -1;
    long long transferred_bytes = 0;

    std::string file_id("");
    std::string job_id(""); //a
    std::string source_url(""); //b
    std::string dest_url(""); //c
    bool overwrite = false; //d
    int nbstreams = 5; //e
    int tcpbuffersize = 0; //f
    int blocksize = 0; //g
    int timeout = 3600; //h
    bool daemonize = true; //i
    std::string dest_token_desc(""); //j
    std::string source_token_desc(""); //k
    int markers_timeout = 180; //l
    int first_marker_timeout = 180; //m
    int srm_get_timeout = 180; //n
    int srm_put_timeout = 180; //o	
    int http_timeout = 180; //p
    bool dont_ping_source = false; //q
    bool dont_ping_dest = false; //r
    bool disable_dir_check = false; //s
    int copy_pin_lifetime = 0; //t
    bool lan_connection = false; //u
    bool fail_nearline = false; //v
    int timeout_per_mb = 0; //w
    int no_progress_timeout = 180; //x
    std::string algorithm(""); //y
    std::string checksum_value(""); //z
    bool compare_checksum = true; //A
    std::string vo("");
    std::string sourceSiteName("");
    std::string destSiteName("");
    char hostname[1024] = {0};
    std::string proxy("");
    char errorBuffer[2048] = {0};

    for (int i(1); i < argc; ++i) {
        std::string temp(argv[i]);
        if (temp.compare("-D") == 0)
            sourceSiteName = std::string(argv[i + 1]);
        if (temp.compare("-E") == 0)
            destSiteName = std::string(argv[i + 1]);
        if (temp.compare("-C") == 0)
            vo = std::string(argv[i + 1]);
        if (temp.compare("-y") == 0)
            algorithm = std::string(argv[i + 1]);
        if (temp.compare("-z") == 0)
            checksum_value = std::string(argv[i + 1]);
        if (temp.compare("-A") == 0)
            compare_checksum = true;
        if (temp.compare("-w") == 0)
            timeout_per_mb = std::atoi(argv[i + 1]);
        if (temp.compare("-x") == 0)
            no_progress_timeout = std::atoi(argv[i + 1]);
        if (temp.compare("-u") == 0)
            lan_connection = true;
        if (temp.compare("-v") == 0)
            fail_nearline = true;
        if (temp.compare("-t") == 0)
            copy_pin_lifetime = std::atoi(argv[i + 1]);
        if (temp.compare("-q") == 0)
            dont_ping_source = true;
        if (temp.compare("-r") == 0)
            dont_ping_dest = true;
        if (temp.compare("-s") == 0)
            disable_dir_check = true;
        if (temp.compare("-a") == 0)
            job_id = std::string(argv[i + 1]);
        if (temp.compare("-b") == 0)
            source_url = std::string(argv[i + 1]);
        if (temp.compare("-c") == 0)
            dest_url = std::string(argv[i + 1]);
        if (temp.compare("-d") == 0)
            overwrite = true;
        if (temp.compare("-e") == 0)
            nbstreams = std::atoi(argv[i + 1]);
        if (temp.compare("-f") == 0)
            tcpbuffersize = std::atoi(argv[i + 1]);
        if (temp.compare("-g") == 0)
            blocksize = std::atoi(argv[i + 1]);
        if (temp.compare("-h") == 0)
            timeout = std::atoi(argv[i + 1]);
        if (temp.compare("-i") == 0)
            daemonize = true;
        if (temp.compare("-j") == 0)
            dest_token_desc = std::string(argv[i + 1]);
        if (temp.compare("-k") == 0)
            source_token_desc = std::string(argv[i + 1]);
        if (temp.compare("-l") == 0)
            markers_timeout = std::atoi(argv[i + 1]);
        if (temp.compare("-m") == 0)
            first_marker_timeout = std::atoi(argv[i + 1]);
        if (temp.compare("-n") == 0)
            srm_get_timeout = std::atoi(argv[i + 1]);
        if (temp.compare("-o") == 0)
            srm_put_timeout = std::atoi(argv[i + 1]);
        if (temp.compare("-p") == 0)
            http_timeout = std::atoi(argv[i + 1]);
        if (temp.compare("-B") == 0)
            file_id = std::string(argv[i + 1]);
        if (temp.compare("-proxy") == 0)
            proxy = std::string(argv[i + 1]);	    
    }       

    seteuid(pw_uid); 	
    UserProxyEnv* cert = NULL;
    if(proxy.length() > 0){
	    // Set Proxy Env    
	    cert = new UserProxyEnv(proxy);
    }
    seteuid(privid);


    fileManagement.setSourceUrl(source_url);
    fileManagement.setDestUrl(dest_url);
    fileManagement.setFileId(file_id);
    fileManagement.setJobId(job_id);
    g_file_id = file_id;
    g_job_id = job_id;

    fileManagement.getLogStream(logStream);
    logger log(logStream);
    
    // register signal SIGINT and signal handler  
    signal(SIGINT, signalHandler);
    
    reporter.constructMessage(job_id, file_id, "ACTIVE", "");          

    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    msg_ifce::getInstance()->set_tr_timestamp_start(&tr_completed, msg_ifce::getInstance()->getTimestamp());
    msg_ifce::getInstance()->set_agent_fqdn(&tr_completed, hostname);

    msg_ifce::getInstance()->set_t_channel(&tr_completed, fileManagement.getSePair());
    msg_ifce::getInstance()->set_transfer_id(&tr_completed, fileManagement.getLogFileName());
    msg_ifce::getInstance()->set_source_srm_version(&tr_completed, "2.2");
    msg_ifce::getInstance()->set_destination_srm_version(&tr_completed, "2.2");
    msg_ifce::getInstance()->set_source_url(&tr_completed, source_url);
    msg_ifce::getInstance()->set_dest_url(&tr_completed, dest_url);
    msg_ifce::getInstance()->set_source_hostname(&tr_completed, fileManagement.getSourceHostname());
    msg_ifce::getInstance()->set_dest_hostname(&tr_completed, fileManagement.getDestHostname());
    msg_ifce::getInstance()->set_channel_type(&tr_completed, "urlcopy");
    msg_ifce::getInstance()->set_vo(&tr_completed, vo);
    msg_ifce::getInstance()->set_source_site_name(&tr_completed, sourceSiteName);
    msg_ifce::getInstance()->set_dest_site_name(&tr_completed, destSiteName);


    std::string nstream_to_string = to_string<int>(nbstreams, std::dec);
    msg_ifce::getInstance()->set_number_of_streams(&tr_completed, nstream_to_string.c_str());

    std::string tcpbuffer_to_string = to_string<int>(tcpbuffersize, std::dec);
    msg_ifce::getInstance()->set_tcp_buffer_size(&tr_completed, tcpbuffer_to_string.c_str());

    std::string block_to_string = to_string<int>(blocksize, std::dec);
    msg_ifce::getInstance()->set_block_size(&tr_completed, block_to_string.c_str());

    std::string timeout_to_string = to_string<int>(timeout, std::dec);
    msg_ifce::getInstance()->set_transfer_timeout(&tr_completed, timeout_to_string.c_str());

    msg_ifce::getInstance()->set_srm_space_token_dest(&tr_completed, dest_token_desc);
    msg_ifce::getInstance()->set_srm_space_token_source(&tr_completed, source_token_desc);
    msg_ifce::getInstance()->SendTransferStartMessage(&tr_completed);
    


    log << fileManagement.timestamp() << "INFO Transfer accepted" << '\n';
    log << fileManagement.timestamp() << "INFO job_id:" << job_id << '\n'; //a
    log << fileManagement.timestamp() << "INFO file_id:" << file_id << '\n'; //a
    log << fileManagement.timestamp() << "INFO source_url:" << source_url << '\n'; //b
    log << fileManagement.timestamp() << "INFO dest_url:" << dest_url << '\n'; //c
    log << fileManagement.timestamp() << "INFO overwrite:" << overwrite << '\n'; //d
    log << fileManagement.timestamp() << "INFO nbstreams:" << nbstreams << '\n'; //e
    log << fileManagement.timestamp() << "INFO tcpbuffersize:" << tcpbuffersize << '\n'; //f
    log << fileManagement.timestamp() << "INFO blocksize:" << blocksize << '\n'; //g
    log << fileManagement.timestamp() << "INFO timeout:" << timeout << '\n'; //h
    log << fileManagement.timestamp() << "INFO daemonize:" << daemonize << '\n'; //i
    log << fileManagement.timestamp() << "INFO dest_token_desc:" << dest_token_desc << '\n'; //j
    log << fileManagement.timestamp() << "INFO source_token_desc:" << source_token_desc << '\n'; //k
    log << fileManagement.timestamp() << "INFO markers_timeout:" << markers_timeout << '\n'; //l
    log << fileManagement.timestamp() << "INFO first_marker_timeout:" << first_marker_timeout << '\n'; //m
    log << fileManagement.timestamp() << "INFO srm_get_timeout:" << srm_get_timeout << '\n'; //n
    log << fileManagement.timestamp() << "INFO srm_put_timeout:" << srm_put_timeout << '\n'; //o	
    log << fileManagement.timestamp() << "INFO http_timeout:" << http_timeout << '\n'; //p
    log << fileManagement.timestamp() << "INFO dont_ping_source:" << dont_ping_source << '\n'; //q
    log << fileManagement.timestamp() << "INFO dont_ping_dest:" << dont_ping_dest << '\n'; //r
    log << fileManagement.timestamp() << "INFO disable_dir_check:" << disable_dir_check << '\n'; //s
    log << fileManagement.timestamp() << "INFO copy_pin_lifetime:" << copy_pin_lifetime << '\n'; //t
    log << fileManagement.timestamp() << "INFO lan_connection:" << lan_connection << '\n'; //u
    log << fileManagement.timestamp() << "INFO fail_nearline:" << fail_nearline << '\n'; //v
    log << fileManagement.timestamp() << "INFO timeout_per_mb:" << timeout_per_mb << '\n'; //w
    log << fileManagement.timestamp() << "INFO no_progress_timeout:" << no_progress_timeout << '\n'; //x
    log << fileManagement.timestamp() << "INFO checksum_value:" << checksum_value << '\n'; //z
    log << fileManagement.timestamp() << "INFO compare_checksum:" << compare_checksum << '\n'; //A
    log << fileManagement.timestamp() << "INFO proxy:" << proxy << '\n'; //proxy

    //reporter.constructMessage(job_id, file_id, "ACTIVE", "");

    msg_ifce::getInstance()->set_time_spent_in_srm_preparation_start(&tr_completed, msg_ifce::getInstance()->getTimestamp());

    seteuid(pw_uid);
     
    if (gfal_stat(source_url.c_str(), &statbufsrc) < 0) {
	std::string tempError = std::string(gfal_posix_strerror_r(errorBuffer, 2048));
        seteuid(privid);
        log << fileManagement.timestamp() << "ERROR Failed to get source file size, errno:" << tempError << '\n';
        errorMessage = "Failed to get source file size: " + tempError;		
	errorScope = SOURCE;
	reasonClass = GENERAL_FAILURE;
	errorPhase = TRANSFER_PREPARATION;	
        goto stop;
    } else {
        seteuid(privid);
        log << fileManagement.timestamp() << "INFO Source file size: " << statbufsrc.st_size << '\n';
        source_size = statbufsrc.st_size;
        //conver longlong to string
        std::string size_to_string = to_string<long double > (source_size, std::dec);
        //set the value of file size to the message
        msg_ifce::getInstance()->set_file_size(&tr_completed, size_to_string.c_str());
    }

    /*Checksuming*/
    if(compare_checksum){
            if(checksum_value.length() > 0){ //user provided checksum
 	        std::vector<std::string> token =  split(checksum_value.c_str());
		std::string checkAlg = token[0];
		std::string csk = token[1];
		seteuid(pw_uid);
	    	gfalt_set_user_defined_checksum(params,checkAlg.c_str(), csk.c_str(), &tmp_err);
		gfalt_set_checksum_check(params, TRUE, &tmp_err);
	    }
	    else{//use auto checksum
		seteuid(pw_uid);
	    	gfalt_set_checksum_check(params, TRUE, &tmp_err);
	    }
    }

    seteuid(pw_uid);
    //overwrite dest file if exists
    if(overwrite){	      
    	gfalt_set_replace_existing_file(params, TRUE, &tmp_err);
    }

    gfalt_set_timeout(params, timeout, NULL);
    gfalt_set_nbstreams(params, nbstreams, NULL);
    gfalt_set_monitor_callback(params, &call_perf, &tmp_err);

    seteuid(privid);
    msg_ifce::getInstance()->set_timestamp_checksum_source_started(&tr_completed, msg_ifce::getInstance()->getTimestamp());
    msg_ifce::getInstance()->set_checksum_timeout(&tr_completed, timeout_to_string.c_str());
    msg_ifce::getInstance()->set_timestamp_checksum_source_ended(&tr_completed, msg_ifce::getInstance()->getTimestamp());


    msg_ifce::getInstance()->set_time_spent_in_srm_preparation_end(&tr_completed, msg_ifce::getInstance()->getTimestamp());

    log << fileManagement.timestamp() << "INFO initialize gfal2" << '\n';
    seteuid(pw_uid);
    if ((handle = gfal_context_new(&tmp_err)) == NULL) {
        seteuid(privid);
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer Initialization failed - errno: " << tmp_err->message << " Error message:" << tmp_err->message << commit;
        log << fileManagement.timestamp() << "ERROR Transfer Initialization failed - errno: " << tmp_err->code << " Error message:" << tmp_err->message << '\n';
        errorMessage = std::string(tmp_err->message);
	errorScope = AGENT;
	reasonClass = GENERAL_FAILURE;
	errorPhase = ALLOCATION;	
        goto stop;
    }
    seteuid(privid);
    log << fileManagement.timestamp() << "INFO begin copy" << '\n';
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << " Transfer Started" << commit;

    msg_ifce::getInstance()->set_timestamp_transfer_started(&tr_completed, msg_ifce::getInstance()->getTimestamp());
    seteuid(pw_uid);
    if ((ret = gfalt_copy_file(handle, params, source_url.c_str(), dest_url.c_str(), &tmp_err)) != 0) {
        seteuid(privid);
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed - errno: " << tmp_err->code << " Error message:" << tmp_err->message << commit;
        log << fileManagement.timestamp() << "ERROR Transfer failed - errno: " << tmp_err->code << " Error message:" << tmp_err->message << '\n';
        errorMessage = std::string(tmp_err->message);
	errorScope = TRANSFER;
	reasonClass = GENERAL_FAILURE;
	errorPhase = TRANSFER;	
        goto stop;
    } else {
        seteuid(privid);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer completed successfully" << commit;
        log << fileManagement.timestamp() << "INFO Transfer completed successfully" << '\n';
    }

    bytes_to_string = to_string<long long>(transferred_bytes, std::dec);
    msg_ifce::getInstance()->set_total_bytes_transfered(&tr_completed, bytes_to_string.c_str());


    msg_ifce::getInstance()->set_timestamp_transfer_completed(&tr_completed, msg_ifce::getInstance()->getTimestamp());

    msg_ifce::getInstance()->set_time_spent_in_srm_finalization_start(&tr_completed, msg_ifce::getInstance()->getTimestamp());


    msg_ifce::getInstance()->set_timestamp_checksum_dest_started(&tr_completed, msg_ifce::getInstance()->getTimestamp());
    seteuid(pw_uid);
    if (gfal_stat(dest_url.c_str(), &statbufdest) < 0) {
        seteuid(privid);
        memset(errorBuffer, 0, 2048);
	std::string tempError = std::string(gfal_posix_strerror_r(errorBuffer, 2048));
        log << fileManagement.timestamp() << "ERROR Failed to get dest file size, errno:" << tempError << '\n';
        errorMessage = "Failed to get dest file size: " + tempError;		
	errorScope = DESTINATION;
	reasonClass = GENERAL_FAILURE;
	errorPhase = TRANSFER_FINALIZATION;	
        goto stop;
    } else {
        seteuid(privid);
        log << fileManagement.timestamp() << "INFO Dest file size: " << statbufdest.st_size << '\n';
        dest_size = statbufdest.st_size;
    }
    msg_ifce::getInstance()->set_timestamp_checksum_dest_ended(&tr_completed, msg_ifce::getInstance()->getTimestamp());


    //check source and dest file sizes
    if (source_size == dest_size) {
        log << fileManagement.timestamp() << "INFO Source and destination files size is the same" << '\n';
    } else {
        log << fileManagement.timestamp() << "ERROR Source and destination file size differ" << '\n';
        errorMessage = "Source and destination file size differ";
	errorScope = DESTINATION;
	reasonClass = GENERAL_FAILURE;
	errorPhase = TRANSFER_FINALIZATION;	
        goto stop;
    }

    log << fileManagement.timestamp() << "INFO release all gfal2 resources" << '\n';
    seteuid(pw_uid);
    gfalt_params_handle_delete(params,NULL);
    gfal_context_free(handle);
    seteuid(privid);
    msg_ifce::getInstance()->set_time_spent_in_srm_finalization_end(&tr_completed, msg_ifce::getInstance()->getTimestamp());

stop:
    seteuid(privid);
    msg_ifce::getInstance()->set_transfer_error_scope(&tr_completed, errorScope);
    msg_ifce::getInstance()->set_transfer_error_category(&tr_completed, reasonClass);
    msg_ifce::getInstance()->set_failure_phase(&tr_completed, errorPhase);
    msg_ifce::getInstance()->set_transfer_error_message(&tr_completed, errorMessage);
    if(errorMessage.length() > 0){
    	msg_ifce::getInstance()->set_final_transfer_state(&tr_completed, "Error");
	reporter.constructMessage(job_id, file_id, "FAILED", errorMessage);
	}
    else{	
	msg_ifce::getInstance()->set_final_transfer_state(&tr_completed, "");    
	reporter.constructMessage(job_id, file_id, "FINISHED", errorMessage);
	}

    if(errorMessage.length() > 0)
    	FTS3_COMMON_LOGGER_NEWLOG(INFO) << errorMessage << commit;	
	
    msg_ifce::getInstance()->set_tr_timestamp_complete(&tr_completed, msg_ifce::getInstance()->getTimestamp());
    msg_ifce::getInstance()->SendTransferFinishMessage(&tr_completed);
    
    logStream.close();
    fileManagement.archive();

    if(cert)
    	delete cert;
	
    return EXIT_SUCCESS;
}
