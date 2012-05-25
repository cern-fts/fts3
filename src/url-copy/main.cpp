#include <iostream>
#include <ctype.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
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

using namespace FTS3_COMMON_NAMESPACE;
using namespace std;

int main(int argc, char **argv) {

    bool terminalTransferState = true;
    std::string errorMessage("");
    
    transfer_completed tr_completed;
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
    }

    FileManagement fileManagement;
    Reporter reporter;
    fileManagement.setSourceUrl(source_url);
    fileManagement.setDestUrl(dest_url);
    fileManagement.setFileId(file_id);
    fileManagement.setJobId(job_id);

    std::ofstream logStream;
    fileManagement.getLogStream(logStream);
    logger log(logStream);

    char hostname[1024]={0};
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


    log << fileManagement.timestamp() << "Transfer accepted" << '\n';
    log << fileManagement.timestamp() << "job_id:" << job_id << '\n'; //a
    log << fileManagement.timestamp() << "file_id:" << file_id << '\n'; //a
    log << fileManagement.timestamp() << "source_url:" << source_url << '\n'; //b
    log << fileManagement.timestamp() << "dest_url:" << dest_url << '\n'; //c
    log << fileManagement.timestamp() << "overwrite:" << overwrite << '\n'; //d
    log << fileManagement.timestamp() << "nbstreams:" << nbstreams << '\n'; //e
    log << fileManagement.timestamp() << "tcpbuffersize:" << tcpbuffersize << '\n'; //f
    log << fileManagement.timestamp() << "blocksize:" << blocksize << '\n'; //g
    log << fileManagement.timestamp() << "timeout:" << timeout << '\n'; //h
    log << fileManagement.timestamp() << "daemonize:" << daemonize << '\n'; //i
    log << fileManagement.timestamp() << "dest_token_desc:" << dest_token_desc << '\n'; //j
    log << fileManagement.timestamp() << "source_token_desc:" << source_token_desc << '\n'; //k
    log << fileManagement.timestamp() << "markers_timeout:" << markers_timeout << '\n'; //l
    log << fileManagement.timestamp() << "first_marker_timeout:" << first_marker_timeout; //m
    log << fileManagement.timestamp() << "srm_get_timeout:" << srm_get_timeout << '\n'; //n
    log << fileManagement.timestamp() << "srm_put_timeout:" << srm_put_timeout << '\n'; //o	
    log << fileManagement.timestamp() << "http_timeout:" << http_timeout << '\n'; //p
    log << fileManagement.timestamp() << "dont_ping_source:" << dont_ping_source << '\n'; //q
    log << fileManagement.timestamp() << "dont_ping_dest:" << dont_ping_dest << '\n'; //r
    log << fileManagement.timestamp() << "disable_dir_check:" << disable_dir_check << '\n'; //s
    log << fileManagement.timestamp() << "copy_pin_lifetime:" << copy_pin_lifetime << '\n'; //t
    log << fileManagement.timestamp() << "lan_connection:" << lan_connection << '\n'; //u
    log << fileManagement.timestamp() << "fail_nearline:" << fail_nearline << '\n'; //v
    log << fileManagement.timestamp() << "timeout_per_mb:" << timeout_per_mb << '\n'; //w
    log << fileManagement.timestamp() << "no_progress_timeout:" << no_progress_timeout << '\n'; //x
    log << fileManagement.timestamp() << "algorithm:" << algorithm << '\n'; //y
    log << fileManagement.timestamp() << "checksum_value:" << checksum_value << '\n'; //z
    log << fileManagement.timestamp() << "compare_checksum:" << compare_checksum << '\n'; //A

    msg_ifce::getInstance()->set_time_spent_in_srm_preparation_start(&tr_completed, msg_ifce::getInstance()->getTimestamp());

    if (gfal_stat(source_url.c_str(), &statbufsrc) < 0) {
        log << fileManagement.timestamp() << "Failed to get source file size, errno: " << gfal_posix_code_error() << '\n';
	terminalTransferState = false;
	errorMessage = "Failed to get source file size";
    } else {
        log << fileManagement.timestamp() << "Source file size: " << statbufsrc.st_size << '\n';
        source_size = statbufsrc.st_size;
        //conver longlong to string
        std::string size_to_string = to_string<long double > (source_size, std::dec);
        //set the value of file size to the message
        msg_ifce::getInstance()->set_file_size(&tr_completed, size_to_string.c_str());	
    }

    //overwrite dest file if exists	      
    gfalt_set_replace_existing_file(params, TRUE, &tmp_err);

    msg_ifce::getInstance()->set_timestamp_checksum_source_started(&tr_completed, msg_ifce::getInstance()->getTimestamp());
    msg_ifce::getInstance()->set_checksum_timeout(&tr_completed, timeout_to_string.c_str());
    msg_ifce::getInstance()->set_timestamp_checksum_source_ended(&tr_completed, msg_ifce::getInstance()->getTimestamp());


    msg_ifce::getInstance()->set_time_spent_in_srm_preparation_end(&tr_completed, msg_ifce::getInstance()->getTimestamp());

    reporter.constructMessage(job_id, file_id, "ACTIVE", "");

    log << fileManagement.timestamp() << "initialize gfal2" << '\n';
    if ((handle = gfal_context_new(&tmp_err)) == NULL) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer Initialization failed - errno: " << tmp_err->message << " Error message:" << tmp_err->message << commit;
        log << fileManagement.timestamp() << "Transfer Initialization failed - errno: " << tmp_err->code << " Error message:" << tmp_err->message << '\n';
	terminalTransferState = false;
	errorMessage = std::string(tmp_err->message);
    }
    log << fileManagement.timestamp() << "begin copy" << '\n';
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << " Transfer Started" << commit;
    
    msg_ifce::getInstance()->set_timestamp_transfer_started(&tr_completed, msg_ifce::getInstance()->getTimestamp());

    if ((ret = gfalt_copy_file(handle, params, source_url.c_str(), dest_url.c_str(), &tmp_err)) != 0) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed - errno: " << tmp_err->code << " Error message:" << tmp_err->message << commit;
        log << fileManagement.timestamp() << "Transfer failed - errno: " << tmp_err->code << " Error message:" << tmp_err->message << '\n';
	terminalTransferState = false;
	errorMessage = std::string(tmp_err->message);
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer completed successfully" << commit;
        log << fileManagement.timestamp() << "Transfer completed successfully" << '\n';
    }

    std::string bytes_to_string = to_string<long long>(transferred_bytes, std::dec);
    msg_ifce::getInstance()->set_total_bytes_transfered(&tr_completed, bytes_to_string.c_str());	


    msg_ifce::getInstance()->set_timestamp_transfer_completed(&tr_completed, msg_ifce::getInstance()->getTimestamp());

    msg_ifce::getInstance()->set_time_spent_in_srm_finalization_start(&tr_completed, msg_ifce::getInstance()->getTimestamp());


    msg_ifce::getInstance()->set_timestamp_checksum_dest_started(&tr_completed, msg_ifce::getInstance()->getTimestamp());
    if (gfal_stat(dest_url.c_str(), &statbufdest) < 0) {
        log << fileManagement.timestamp() << "Failed to get destination file size, errno: " << gfal_posix_code_error() << '\n';
	terminalTransferState = false;
	errorMessage = "Failed to get destination file size";
    } else {
        log << fileManagement.timestamp() << "Dest file size: " << statbufdest.st_size << '\n';
        dest_size = statbufdest.st_size;
    }
    msg_ifce::getInstance()->set_timestamp_checksum_dest_ended(&tr_completed, msg_ifce::getInstance()->getTimestamp());


    //check source and dest file sizes
    if (source_size == dest_size){
        log << fileManagement.timestamp() << "Source and destination files size is the same" << '\n';
    }
    else{
        log << fileManagement.timestamp() << "Source and destination file size differ" << '\n';
	terminalTransferState = false;
	errorMessage = "Source and destination file size differ";
    }

    log << fileManagement.timestamp() << "release all gfal2 resources" << '\n';
    gfal_context_free(handle);

    msg_ifce::getInstance()->set_time_spent_in_srm_finalization_end(&tr_completed, msg_ifce::getInstance()->getTimestamp());

    msg_ifce::getInstance()->set_transfer_error_scope(&tr_completed, "SOURCE");
    msg_ifce::getInstance()->set_transfer_error_category(&tr_completed, "category");
    msg_ifce::getInstance()->set_transfer_error_message(&tr_completed, "error message");
    msg_ifce::getInstance()->set_failure_phase(&tr_completed, "PHASE_PREPARATION");
    msg_ifce::getInstance()->set_final_transfer_state(&tr_completed, "Error");  
    msg_ifce::getInstance()->set_tr_timestamp_complete(&tr_completed, msg_ifce::getInstance()->getTimestamp());    
    msg_ifce::getInstance()->SendTransferFinishMessage(&tr_completed);
   
    reporter.constructMessage(job_id, file_id, "FINISHED", errorMessage);

    logStream.close();
    fileManagement.archive();

    return EXIT_SUCCESS;
}
