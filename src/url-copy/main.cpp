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


#include <iostream>
#include <algorithm>
#include <ctype.h>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <stdio.h>
#include <pwd.h>
#include <transfer/gfal_transfer.h>
#include <fstream>
#include "boost/date_time/gregorian/gregorian.hpp"
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
#include <boost/tokenizer.hpp>
#include <cstdio>
#include <ctime>
#include "definitions.h"
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <exception>
#include "monitoring/utility_routines.h"
#include <ctime>
#include "name_to_uid.h"
#include <boost/thread.hpp>  
#include "StaticSslLocking.h"
#include <sys/resource.h>


using namespace std;
using boost::thread;

FileManagement* fileManagement = NULL;
static Reporter reporter;
static std::ofstream logStream;
static transfer_completed tr_completed;
static bool retry = true;
static std::string strArray[7];
static std::string g_file_id("0");
static std::string g_job_id("");
static std::string errorScope("");
static std::string errorPhase("");
static std::string reasonClass("");
static std::string errorMessage("");
static std::string readFile("");
static std::string reuseFile("");
static std::string token_bringonline("");
static double source_size = 0.0;
static double dest_size = 0.0;
static std::string userFilesize("");
static double diff = 0.0;
static double transfer_start = 0.0;
static double transfer_complete = 0.0;
static uid_t pw_uid;
static std::string file_id("0");
static std::string job_id(""); //a
static std::string source_url(""); //b
static std::string dest_url(""); //c
static bool overwrite = false; //d
static unsigned int nbstreams = DEFAULT_NOSTREAMS; //e
static unsigned int tcpbuffersize = DEFAULT_BUFFSIZE; //f
static unsigned int blocksize = 0; //g
static unsigned int timeout = DEFAULT_TIMEOUT; //h
static bool daemonize = true; //i
static std::string dest_token_desc(""); //j
static std::string source_token_desc(""); //k
static unsigned int markers_timeout = 180; //l
static unsigned int first_marker_timeout = 180; //m
static unsigned int srm_get_timeout = 180; //n
static unsigned int srm_put_timeout = 180; //o	
static unsigned int http_timeout = 180; //p
static bool dont_ping_source = false; //q
static bool dont_ping_dest = false; //r
static bool disable_dir_check = false; //s
static int copy_pin_lifetime = -1; //t
static int bringonline = -1; //H
static bool lan_connection = false; //u
static bool fail_nearline = false; //v
static unsigned int timeout_per_mb = 0; //w
static unsigned int no_progress_timeout = 180; //x
static std::string algorithm(""); //y
static std::string checksum_value(""); //z
static bool compare_checksum = false; //A
static std::string vo("");
static std::string sourceSiteName("");
static std::string destSiteName("");
static char hostname[1024] = {0};
static std::string proxy("");
static bool debug = false;
static volatile bool propagated = false;
static volatile bool canceled = false;
static volatile bool terminalState = false;
static std::string nstream_to_string("");
static std::string tcpbuffer_to_string("");
static std::string block_to_string("");
static std::string timeout_to_string("");
static std::string file_Metadata("");
static std::string job_Metadata(""); //a
static std::string globalErrorMessage("");
static std::string infosys("");

extern std::string stackTrace;
gfal_context_t handle = NULL;


static void setStackLimits(){
	struct rlimit rl;
	rl.rlim_cur = 1048576; //1MB
	rl.rlim_max = 2097152; //2MB
	setrlimit(RLIMIT_STACK, &rl);
}

//convert milli to secs
static double transferDuration(double start , double complete){
	return (start==0 && complete==0)==true? 0.0: (complete - start) / 1000; 
}


//categories are: source, destination, transfer
static bool retryTransfer(int errorNo, std::string category ){
	bool retry = true;
			
	if(category=="SOURCE"){
		switch ( errorNo ) {
			case ENOENT:
			  retry = false;
			  break;
			case EPERM:
			  retry = false;
			  break;
			case EACCES:
			  retry = false;
			  break;			
			case EISDIR:
			  retry = false;
			  break;
			case ENAMETOOLONG:
			  retry = false;
			  break;				  				  
			default:
			  retry = true;
			  break;
			}		
	}else if(category=="DESTINATION"){
  	        switch ( errorNo ) {
			case ENOENT:
			  retry = false;
			  break;
			case EPERM:
			  retry = false;
			  break;
			case EACCES:
			  retry = false;
			  break;
			case EISDIR:
			  retry = false;
			  break;
			case ENAMETOOLONG:
			  retry = false;
			  break;					  				  
			default:
			  retry = true;
			  break;
		}
	}else{ //TRANSFER
		switch ( errorNo ) {	
			case ENOSPC:
			  retry = false;
			  break;
			case EPERM:
			  retry = false;
			  break;
			case EACCES:
			  retry = false;
			  break;
			case EEXIST:
			  retry = false;
			  break;
			case EFBIG:
			  retry = false;
			  break;
			case EROFS:
			  retry = false;
			  break;
			case ENAMETOOLONG:
			  retry = false;
			  break;				  
			default:
			  retry = true;
			  break;
		}			
	}
	
	return retry;
}

/*hardcoded for now*/
static unsigned int adjustStreamsBasedOnSize(off_t sizeInBytes, unsigned int currentStreams){
	if(sizeInBytes <= 20971520) {
		return 1;
	}
	else if(sizeInBytes >= 20971520 && sizeInBytes <= 104857600){
		return 2;
	}
	else if (sizeInBytes >= 104857600 && sizeInBytes <= 524288000){
		return 3;
	}
	else{
		if(currentStreams < 4 )
			return 4;
		else
			return currentStreams;
	}
 return 2;	
}

static void cancelTransfer() {
    if (handle && !canceled) { // finish all transfer in a clean way
        canceled = true;
        gfal2_cancel(handle);
    }
}

static int fexists(const char *filename) {
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

static std::string srmVersion(const std::string & url) {
    if (url.compare(0, 6, "srm://") == 0)
        return std::string("2.2.0");

    return std::string("");
}

/*replace space with _*/
static std::string mapErrnoToString(int err) {
    if (err != 0) {
        char buf[256] = {0};
        char const * str = strerror_r(err, buf, 256);
        if (str) {
            std::string rep(str);
            std::replace(rep.begin(), rep.end(), ' ', '_');
            return boost::to_upper_copy(rep);
        }
    }
    return "GENERAL ERROR";
}

static std::vector<std::string> split(const char *str, char c = ':') {
    std::vector<std::string> result;

    while (1) {
        const char *begin = str;

        while (*str != c && *str)
            str++;

        result.push_back(string(begin, str));

        if (0 == *str++)
            break;
    }

    return result;
}

static void call_perf(gfalt_transfer_status_t h, const char*, const char*, gpointer) {
    if (h) {
        size_t avg = gfalt_copy_get_average_baudrate(h, NULL);
        if (avg > 0) {
            avg = avg / 1024;
        } else {
            avg = 0;
        }
        size_t inst = gfalt_copy_get_instant_baudrate(h, NULL);
        if (inst > 0) {
            inst = inst / 1024;
        } else {
            inst = 0;
        }

        size_t trans = gfalt_copy_get_bytes_transfered(h, NULL);
        time_t elapsed = gfalt_copy_get_elapsed_time(h, NULL);
        logStream << fileManagement->timestamp() << "INFO bytes:" << trans << ", avg KB/sec :" << avg << ", inst KB/sec :" << inst << ", elapsed:" << elapsed << '\n';
    }

}

std::string getDefaultScope() {
    return errorScope.length() == 0 ? AGENT : errorScope;
}

std::string getDefaultReasonClass() {
    return reasonClass.length() == 0 ? ALLOCATION : reasonClass;
}

std::string getDefaultErrorPhase() {
    return errorPhase.length() == 0 ? GENERAL_FAILURE : errorPhase;
}

void abnormalTermination(const std::string& classification, const std::string&, const std::string& finalState) {
    terminalState = true;

    if(globalErrorMessage.length() > 0){
    	errorMessage += " " + globalErrorMessage;
    }
    
    diff = transferDuration(transfer_start, transfer_complete);
    msg_ifce::getInstance()->set_transfer_error_scope(&tr_completed, getDefaultScope());
    msg_ifce::getInstance()->set_transfer_error_category(&tr_completed, getDefaultReasonClass());
    msg_ifce::getInstance()->set_failure_phase(&tr_completed, getDefaultErrorPhase());
    msg_ifce::getInstance()->set_transfer_error_message(&tr_completed, errorMessage);
    msg_ifce::getInstance()->set_final_transfer_state(&tr_completed, finalState);
    msg_ifce::getInstance()->set_tr_timestamp_complete(&tr_completed, msg_ifce::getInstance()->getTimestamp());
    msg_ifce::getInstance()->SendTransferFinishMessage(&tr_completed);

    reporter.timeout = timeout;
    reporter.nostreams = nbstreams;
    reporter.buffersize = tcpbuffersize;
    
    if (strArray[0].length() > 0)
        reporter.constructMessage(retry, g_job_id, strArray[0], classification, errorMessage, diff, source_size);
    else
        reporter.constructMessage(retry, g_job_id, g_file_id, classification, errorMessage, diff, source_size);

    std::string moveFile = fileManagement->archive();
    if (moveFile.length() != 0) {
        logStream << fileManagement->timestamp() << "ERROR Failed to archive file: " << moveFile << '\n';
    }
    if (reuseFile.length() > 0)
        unlink(readFile.c_str());

    cancelTransfer();
    sleep(1);
    exit(1);
}

void canceler() {   
        errorMessage = "WARN Transfer " + g_job_id + " was canceled because it was not responding";
        logStream << fileManagement->timestamp() << errorMessage << '\n';
        abnormalTermination("FAILED", errorMessage, "Abort");	
}

void taskTimer(int time) {
    boost::this_thread::sleep(boost::posix_time::seconds(time));
    canceler();
}

void taskStatusUpdater(int time) {
    while (1) {
        if (strArray[0].length() > 0) {
	    logStream << fileManagement->timestamp() << "INFO Sending back to the server url-copy is still alive!" << '\n';
            reporter.constructMessageUpdater(job_id, strArray[0]);
        } else {
	    logStream << fileManagement->timestamp() << "INFO Sending back to the server url-copy is still alive!" << '\n';	
            reporter.constructMessageUpdater(job_id, file_id);
        }
        boost::this_thread::sleep(boost::posix_time::seconds(time));
    }
}

void signalHandler(int signum) {
    logStream << fileManagement->timestamp() << "DEBUG Received signal " << signum << '\n';
    if (stackTrace.length() > 0) {
        propagated = true;
        errorMessage = "ERROR Transfer process died " + g_job_id;
        logStream << fileManagement->timestamp() << errorMessage << '\n';
        logStream << fileManagement->timestamp() << "ERROR " << stackTrace << '\n';
	errorMessage += stackTrace;
        abnormalTermination("FAILED", errorMessage, "Error");
    } else if (signum == 2) {
        if (propagated == false) {
            propagated = true;
            errorMessage = "WARN Transfer " + g_job_id + " canceled by the user";
            logStream << fileManagement->timestamp() << errorMessage << '\n';
            abnormalTermination("CANCELED", errorMessage, "Abort");
        }
    } else if (signum == 10) {
        if (propagated == false) {
            propagated = true;
            errorMessage = "WARN Transfer " + g_job_id + " has been forced-canceled because it was stalled";
            logStream << fileManagement->timestamp() << errorMessage << '\n';
            abnormalTermination("FAILED", errorMessage, "Abort");
        }
    } else {
        if (propagated == false) {
            propagated = true;
            errorMessage = "WARN Transfer " + g_job_id + " canceled";
            logStream << fileManagement->timestamp() << errorMessage << '\n';
            abnormalTermination("CANCELED", errorMessage, "Abort");
        }
    }
}

// Callback used to populate the messaging with the different stages

static void event_logger(const gfalt_event_t e, gpointer udata) {
    static const char* sideStr[] = {"SRC", "DST", "BTH"};
    static const GQuark SRM_DOMAIN = g_quark_from_static_string("SRM");

    msg_ifce* msg = msg_ifce::getInstance();
    std::string timestampStr = _to_string<long double>(e->timestamp, std::dec);

    logger* log = static_cast<logger*> (udata);

    if (log && debug) {
        (*log) << fileManagement->timestamp() << "DEBUG "
                << '[' << timestampStr << "] "
                << sideStr[e->side] << ' '
                << g_quark_to_string(e->domain) << '\t'
                << g_quark_to_string(e->stage) << '\t'
                << e->description << '\n';
    }

    if (e->stage == GFAL_EVENT_TRANSFER_ENTER){
        msg->set_timestamp_transfer_started(&tr_completed, timestampStr);
	transfer_start = e->timestamp;	
    }else if (e->stage == GFAL_EVENT_TRANSFER_EXIT){
        msg->set_timestamp_transfer_completed(&tr_completed, timestampStr);	   
	transfer_complete = e->timestamp;		
    }

    else if (e->stage == GFAL_EVENT_CHECKSUM_ENTER && e->side == GFAL_EVENT_SOURCE)
        msg->set_timestamp_checksum_source_started(&tr_completed, timestampStr);
    else if (e->stage == GFAL_EVENT_CHECKSUM_EXIT && e->side == GFAL_EVENT_SOURCE)
        msg->set_timestamp_checksum_source_ended(&tr_completed, timestampStr);

    else if (e->stage == GFAL_EVENT_CHECKSUM_ENTER && e->side == GFAL_EVENT_DESTINATION)
        msg->set_timestamp_checksum_dest_started(&tr_completed, timestampStr);
    else if (e->stage == GFAL_EVENT_CHECKSUM_EXIT && e->side == GFAL_EVENT_DESTINATION)
        msg->set_timestamp_checksum_dest_ended(&tr_completed, timestampStr);

    else if (e->stage == GFAL_EVENT_PREPARE_ENTER && e->domain == SRM_DOMAIN)
        msg->set_time_spent_in_srm_preparation_start(&tr_completed, timestampStr);
    else if (e->stage == GFAL_EVENT_PREPARE_EXIT && e->domain == SRM_DOMAIN)
        msg->set_time_spent_in_srm_preparation_end(&tr_completed, timestampStr);

    else if (e->stage == GFAL_EVENT_CLOSE_ENTER && e->domain == SRM_DOMAIN)
        msg->set_time_spent_in_srm_finalization_start(&tr_completed, timestampStr);
    else if (e->stage == GFAL_EVENT_CLOSE_EXIT && e->domain == SRM_DOMAIN)
        msg->set_time_spent_in_srm_finalization_end(&tr_completed, timestampStr);
}

static void log_func(const gchar *, GLogLevelFlags, const gchar *message, gpointer) {
    if (message) {
        logStream << fileManagement->timestamp() << "DEBUG " << message << '\n';
    }
}

__attribute__((constructor)) void begin(void) {
    //switch to non-priviledged user to avoid reading the hostcert
    pw_uid = name_to_uid();
    setuid(pw_uid);
    seteuid(pw_uid);   
    setStackLimits();     
}

int main(int argc, char **argv) {

    REGISTER_SIGNAL(SIGABRT);
    REGISTER_SIGNAL(SIGSEGV);
    REGISTER_SIGNAL(SIGTERM);
    REGISTER_SIGNAL(SIGILL);
    REGISTER_SIGNAL(SIGFPE);
    REGISTER_SIGNAL(SIGBUS);
    REGISTER_SIGNAL(SIGTRAP);
    REGISTER_SIGNAL(SIGSYS);
    REGISTER_SIGNAL(SIGUSR1);

    // register signal SIGINT & SIGUSR1signal handler  
    signal(SIGINT, signalHandler);
    signal(SIGUSR1, signalHandler);

    for (register int i(1); i < argc; ++i) {
        std::string temp(argv[i]);
        if (temp.compare("-M") == 0)
            infosys = std::string(argv[i + 1]);	
        if (temp.compare("-L") == 0)
            token_bringonline = std::string(argv[i + 1]);	
        if (temp.compare("-K") == 0)
            file_Metadata = std::string(argv[i + 1]);
        if (temp.compare("-J") == 0)
            job_Metadata = std::string(argv[i + 1]);
        if (temp.compare("-I") == 0)
            userFilesize = std::string(argv[i + 1]);
        if (temp.compare("-H") == 0)
            bringonline = boost::lexical_cast<int>(argv[i + 1]);
        if (temp.compare("-G") == 0)
            reuseFile = std::string(argv[i + 1]);
        if (temp.compare("-F") == 0)
            debug = true;
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
            timeout_per_mb = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-x") == 0)
            no_progress_timeout = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-u") == 0)
            lan_connection = true;
        if (temp.compare("-v") == 0)
            fail_nearline = true;
        if (temp.compare("-t") == 0)
            copy_pin_lifetime = boost::lexical_cast<int>(argv[i + 1]);
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
            nbstreams = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-f") == 0)
            tcpbuffersize = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-g") == 0)
            blocksize = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-h") == 0)
            timeout = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-i") == 0)
            daemonize = true;
        if (temp.compare("-j") == 0)
            dest_token_desc = std::string(argv[i + 1]);
        if (temp.compare("-k") == 0)
            source_token_desc = std::string(argv[i + 1]);
        if (temp.compare("-l") == 0)
            markers_timeout = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-m") == 0)
            first_marker_timeout = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-n") == 0)
            srm_get_timeout = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-o") == 0)
            srm_put_timeout = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-p") == 0)
            http_timeout = boost::lexical_cast<unsigned int>(argv[i + 1]);
        if (temp.compare("-B") == 0)
            file_id = std::string(argv[i + 1]);
        if (temp.compare("-proxy") == 0)
            proxy = std::string(argv[i + 1]);
    }

    g_file_id = file_id;
    g_job_id = job_id;
    
    CRYPTO_malloc_init(); // Initialize malloc, free, etc for OpenSSL's use 
    SSL_library_init(); // Initialize OpenSSL's SSL libraries 
    SSL_load_error_strings(); // Load SSL error strings 
    ERR_load_BIO_strings(); // Load BIO error strings 
    StaticSslLocking::init_locks();     
    
    
    fileManagement = new FileManagement(infosys);
    
    std::string bytes_to_string("");
    struct stat statbufsrc;
    struct stat statbufdest;
    GError *tmp_err = NULL; // classical GError/glib error management   
    gfalt_params_t params;
    params = gfalt_params_handle_new(NULL);

    gfalt_set_event_callback(params, event_logger, NULL);

    int ret = -1;
    long long transferred_bytes = 0;
    UserProxyEnv* cert = NULL;

    hostname[1023] = '\0';
    gethostname(hostname, 1023);    


    /*TODO: until we find a way to calculate RTT(perfsonar) accurately, OS tcp auto-tuning does a better job*/
    tcpbuffersize = DEFAULT_BUFFSIZE;
    
    if(argc < 4){
        errorMessage = "Failed to read url-copy process arguments";
    	abnormalTermination("FAILED", errorMessage, "Abort");
    }


   try{
    /*send an update message back to the server to indicate it's alive*/
    boost::thread btUpdater(taskStatusUpdater, 30);
    }catch (std::exception& e) {
    	globalErrorMessage = e.what();
	throw;
    }catch(...){
        globalErrorMessage = "Failed to create boost thread, boost::thread_resource_error";
	throw;
    }

    if (proxy.length() > 0) {
        // Set Proxy Env    
        cert = new UserProxyEnv(proxy);
    }

    GError* handleError = NULL;    
    handle = gfal_context_new(&handleError); 

    if (!handle) {
        errorMessage = "Failed to create the gfal2 handle: ";
        if (handleError && handleError->message) {
            errorMessage += handleError->message;
            abnormalTermination("FAILED", errorMessage, "Error");
        }
    }

    //reuse session    
    if (reuseFile.length() > 0) {
        gfal2_set_opt_boolean(handle, "GRIDFTP PLUGIN", "SESSION_REUSE", TRUE, NULL);
    }

    std::vector<std::string> urlsFile;
    std::string line("");
    readFile = "/var/lib/fts3/" + job_id;
    if (reuseFile.length() > 0) {
        std::ifstream infile(readFile.c_str(), std::ios_base::in);
        while (getline(infile, line, '\n')) {
            urlsFile.push_back(line);
        }
        infile.close();
        unlink(readFile.c_str());
    }

    //cancelation point 
    long unsigned int reuseOrNot = (urlsFile.empty() == true) ? 1 : urlsFile.size();
    unsigned timerTimeout = reuseOrNot * (http_timeout + srm_put_timeout + srm_get_timeout + timeout);
    
    try{
    	boost::thread bt(taskTimer, timerTimeout);
    }catch (std::exception& e) {
    	globalErrorMessage = e.what();
	throw;
    }catch(...){
        globalErrorMessage = "Failed to create boost thread, boost::thread_resource_error";
	throw;
    }

    if (reuseFile.length() > 0 && urlsFile.empty() == true) {
        errorMessage = "Transfer " + g_job_id + " containes no urls with session reuse enabled";

        abnormalTermination("FAILED", errorMessage, "Error");
    }


    for (register unsigned int ii = 0; ii < reuseOrNot; ii++) {
        errorScope = std::string("");
        reasonClass = std::string("");
        errorPhase = std::string("");
	retry = true;

        if (reuseFile.length() > 0) {
            std::string mid_str(urlsFile[ii]);
            typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
            tokenizer tokens(mid_str, boost::char_separator<char> (" "));
            std::copy(tokens.begin(), tokens.end(), strArray);
        } else {
            strArray[0] = file_id;
            strArray[1] = source_url;
            strArray[2] = dest_url;
            strArray[3] = checksum_value;
	    if(userFilesize.length() > 0)
            	strArray[4] = userFilesize;
	    else
	        strArray[4] = "0";
            strArray[5] = file_Metadata;
	    strArray[6] = token_bringonline;
        }


        fileManagement->setSourceUrl(strArray[1]);
        fileManagement->setDestUrl(strArray[2]);
        fileManagement->setFileId(strArray[0]);
        fileManagement->setJobId(job_id);
        g_file_id = strArray[0];
        g_job_id = job_id;

        reporter.timeout = timeout;
        reporter.nostreams = nbstreams;
        reporter.buffersize = tcpbuffersize;
        reporter.source_se = fileManagement->getSourceHostname();
        reporter.dest_se = fileManagement->getDestHostname();
        fileManagement->generateLogFile();

        msg_ifce::getInstance()->set_tr_timestamp_start(&tr_completed, msg_ifce::getInstance()->getTimestamp());
        msg_ifce::getInstance()->set_agent_fqdn(&tr_completed, hostname);
        msg_ifce::getInstance()->set_t_channel(&tr_completed, fileManagement->getSePair());
        msg_ifce::getInstance()->set_transfer_id(&tr_completed, fileManagement->getLogFileName());
        msg_ifce::getInstance()->set_source_srm_version(&tr_completed, srmVersion(strArray[1]));
        msg_ifce::getInstance()->set_destination_srm_version(&tr_completed, srmVersion(strArray[2]));
        msg_ifce::getInstance()->set_source_url(&tr_completed, strArray[1]);
        msg_ifce::getInstance()->set_dest_url(&tr_completed, strArray[2]);
        msg_ifce::getInstance()->set_source_hostname(&tr_completed, fileManagement->getSourceHostname());
        msg_ifce::getInstance()->set_dest_hostname(&tr_completed, fileManagement->getDestHostname());
        msg_ifce::getInstance()->set_channel_type(&tr_completed, "urlcopy");
        msg_ifce::getInstance()->set_vo(&tr_completed, vo);
        msg_ifce::getInstance()->set_source_site_name(&tr_completed, sourceSiteName);
        msg_ifce::getInstance()->set_dest_site_name(&tr_completed, destSiteName);
        nstream_to_string = to_string<unsigned int>(nbstreams, std::dec);
        msg_ifce::getInstance()->set_number_of_streams(&tr_completed, nstream_to_string.c_str());
        tcpbuffer_to_string = to_string<unsigned int>(tcpbuffersize, std::dec);
        msg_ifce::getInstance()->set_tcp_buffer_size(&tr_completed, tcpbuffer_to_string.c_str());
        block_to_string = to_string<unsigned int>(blocksize, std::dec);
        msg_ifce::getInstance()->set_block_size(&tr_completed, block_to_string.c_str());
        timeout_to_string = to_string<unsigned int>(timeout, std::dec);
        msg_ifce::getInstance()->set_transfer_timeout(&tr_completed, timeout_to_string.c_str());
        msg_ifce::getInstance()->set_srm_space_token_dest(&tr_completed, dest_token_desc);
        msg_ifce::getInstance()->set_srm_space_token_source(&tr_completed, source_token_desc);
        msg_ifce::getInstance()->SendTransferStartMessage(&tr_completed);	

        int checkError = fileManagement->getLogStream(logStream);
        if (checkError != 0) {
            std::string message = mapErrnoToString(checkError);
            errorMessage = "Failed to create transfer log file, error was: " + message;
            goto stop;
        }
        { //add curly brackets to delimit the scope of the logger
            logger log(logStream);

            gfalt_set_user_data(params, &log, NULL);

            log << fileManagement->timestamp() << "INFO Transfer accepted" << '\n';
            log << fileManagement->timestamp() << "INFO Proxy:" << proxy << '\n';
            log << fileManagement->timestamp() << "INFO VO:" << vo << '\n'; //a
            log << fileManagement->timestamp() << "INFO Job id:" << job_id << '\n'; //a
            log << fileManagement->timestamp() << "INFO File id:" << strArray[0] << '\n'; //a
            log << fileManagement->timestamp() << "INFO Source url:" << strArray[1] << '\n'; //b
            log << fileManagement->timestamp() << "INFO Dest url:" << strArray[2] << '\n'; //c
            log << fileManagement->timestamp() << "INFO Overwrite enabled:" << overwrite << '\n'; //d
            log << fileManagement->timestamp() << "INFO nbstreams:" << nbstreams << '\n'; //e
            log << fileManagement->timestamp() << "INFO tcpbuffersize:" << tcpbuffersize << '\n'; //f
            log << fileManagement->timestamp() << "INFO blocksize:" << blocksize << '\n'; //g
            log << fileManagement->timestamp() << "INFO Timeout:" << timeout << '\n'; //h
            log << fileManagement->timestamp() << "INFO Daemonize:" << daemonize << '\n'; //i
            log << fileManagement->timestamp() << "INFO Dest space token:" << dest_token_desc << '\n'; //j
            log << fileManagement->timestamp() << "INFO Sourcespace token:" << source_token_desc << '\n'; //k
            log << fileManagement->timestamp() << "INFO markers_timeout:" << markers_timeout << '\n'; //l
            log << fileManagement->timestamp() << "INFO first_marker_timeout:" << first_marker_timeout << '\n'; //m
            log << fileManagement->timestamp() << "INFO srm_get_timeout:" << srm_get_timeout << '\n'; //n
            log << fileManagement->timestamp() << "INFO srm_put_timeout:" << srm_put_timeout << '\n'; //o
            log << fileManagement->timestamp() << "INFO http_timeout:" << http_timeout << '\n'; //p
            log << fileManagement->timestamp() << "INFO dont_ping_source:" << dont_ping_source << '\n'; //q
            log << fileManagement->timestamp() << "INFO dont_ping_dest:" << dont_ping_dest << '\n'; //r
            log << fileManagement->timestamp() << "INFO disable_dir_check:" << disable_dir_check << '\n'; //s
            log << fileManagement->timestamp() << "INFO copy_pin_lifetime:" << copy_pin_lifetime << '\n'; //t
            log << fileManagement->timestamp() << "INFO bringOnline:" << bringonline << '\n'; //t	    
            log << fileManagement->timestamp() << "INFO lan_connection:" << lan_connection << '\n'; //u
            log << fileManagement->timestamp() << "INFO fail_nearline:" << fail_nearline << '\n'; //v
            log << fileManagement->timestamp() << "INFO timeout_per_mb:" << timeout_per_mb << '\n'; //w
            log << fileManagement->timestamp() << "INFO no_progress_timeout:" << no_progress_timeout << '\n'; //x
            log << fileManagement->timestamp() << "INFO Checksum:" << strArray[3] << '\n'; //z
            log << fileManagement->timestamp() << "INFO Checksum enabled:" << compare_checksum << '\n'; //A
            log << fileManagement->timestamp() << "INFO User specified filesize:" << strArray[4] << '\n'; //A
            log << fileManagement->timestamp() << "INFO File metadata:" << strArray[5] << '\n'; //A
            log << fileManagement->timestamp() << "INFO Job metadata:" << job_Metadata << '\n'; //A
	    log << fileManagement->timestamp() << "INFO Bringonline token:" << strArray[6] << '\n'; //A
	    
	    log << fileManagement->timestamp() << "INFO Send transfer start message to monitoring" << '\n';	    	
	
	    /*
            if ( errCode==ENOENT && (bringonline > 0 || copy_pin_lifetime > 0) && isSrmUrl(strArray[1])) { 
                if (gfal2_bring_online(handle, (strArray[1]).c_str(), copy_pin_lifetime, bringonline,
                                       stagingToken, 0, sizeof(stagingToken), &tmp_err) < 0) {
                    std::string tempError(tmp_err->message);
                    const int errCode = tmp_err->code;
                    log << fileManagement->timestamp() << "ERROR Failed to stage file, errno:" << tempError << '\n';
                    errorMessage = "Failed to stage file: " + tempError;
                    errorScope = SOURCE;
                    reasonClass = mapErrnoToString(errCode);
                    errorPhase = TRANSFER_PREPARATION;
                    g_clear_error(&tmp_err);
                    goto stop;
                }
                //staging finished without failure
		log << fileManagement->timestamp() << "INFO Staging file" << source_size << " finished" << '\n';
            }
	    */


            //set to active
            log << fileManagement->timestamp() << "INFO Set the transfer to ACTIVE, report back to the server" << '\n';
            reporter.constructMessage(false, job_id, strArray[0], "ACTIVE", "", diff, source_size);	    
	    
	    	  	   
            if (fexists(proxy.c_str()) != 0) {
                errorMessage = "ERROR proxy doesn't exist, probably expired and not renewed " + proxy;
                errorScope = SOURCE;
                reasonClass = mapErrnoToString(errno);
                errorPhase = TRANSFER_PREPARATION;
                log << fileManagement->timestamp() << errorMessage << '\n';
                goto stop;
            }

            /*set infosys to gfal2*/
            if (handle) {
                char *bdii = (char *) fileManagement->getBDII().c_str();
                if (bdii) {
                    log << fileManagement->timestamp() << "INFO BDII:" << bdii << '\n';
                    if (std::string(bdii).compare("false") == 0) {
                        gfal2_set_opt_boolean(handle, "BDII", "ENABLED", false, NULL);
                    } else {
                        gfal2_set_opt_string(handle, "BDII", "LCG_GFAL_INFOSYS", bdii, NULL);
                    }
                }
            }

            /*gfal2 debug logging*/
            if (debug == true) {
                log << fileManagement->timestamp() << "INFO Set the transfer to debug mode" << '\n';
                gfal_set_verbose(GFAL_VERBOSE_TRACE | GFAL_VERBOSE_VERBOSE | GFAL_VERBOSE_TRACE_PLUGIN);
                FILE* reopenDebugFile = freopen(fileManagement->getLogFileFullPath().c_str(), "w", stderr);
                chmod(fileManagement->getLogFileFullPath().c_str(), (mode_t) 0644);
                if (reopenDebugFile == NULL) {
                    log << fileManagement->timestamp() << "WARN Failed to create debug file, errno:" << mapErrnoToString(errno) << '\n';
                }
                gfal_log_set_handler((GLogFunc) log_func, NULL);
            }

            if (source_token_desc.length() > 0)
                gfalt_set_src_spacetoken(params, source_token_desc.c_str(), NULL);

            if (dest_token_desc.length() > 0)
                gfalt_set_dst_spacetoken(params, dest_token_desc.c_str(), NULL);

            gfalt_set_create_parent_dir(params, TRUE, NULL);

            //get checksum timeout from gfal2
            log << fileManagement->timestamp() << "INFO get checksum timeout" << '\n';
            int checksumTimeout = gfal2_get_opt_integer(handle, "GRIDFTP PLUGIN", "CHECKSUM_CALC_TIMEOUT", NULL);
            msg_ifce::getInstance()->set_checksum_timeout(&tr_completed, boost::lexical_cast<std::string > (checksumTimeout));

            log << fileManagement->timestamp() << "INFO Stat the source surl start" << '\n';
            for (int sourceStatRetry = 0; sourceStatRetry < 4; sourceStatRetry++) {
                if (gfal2_stat(handle, (strArray[1]).c_str(), &statbufsrc, &tmp_err) < 0) {
                    std::string tempError(tmp_err->message);
                    const int errCode = tmp_err->code;
                    log << fileManagement->timestamp() << "ERROR Failed to get source file size, errno:" << tempError << '\n';
                    errorMessage = "Failed to get source file size: " + tempError;
                    errorScope = SOURCE;
                    reasonClass = mapErrnoToString(errCode);
                    errorPhase = TRANSFER_PREPARATION;
		    retry = retryTransfer(tmp_err->code, "SOURCE" );
                    if (sourceStatRetry == 3 || retry == false){
  			log << fileManagement->timestamp() << "INFO No more retries for stat the source" << '\n';
                        g_clear_error(&tmp_err);
                        goto stop;
		    }
                  g_clear_error(&tmp_err);
                } else {
                    if (statbufsrc.st_size <= 0) {
                        errorMessage = "Source file size is 0";
                        log << fileManagement->timestamp() << "ERROR " << errorMessage << '\n';
                        errorScope = SOURCE;
                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                        errorPhase = TRANSFER_PREPARATION;
                        if (sourceStatRetry == 3){
		            log << fileManagement->timestamp() << "INFO No more retries for stat the source" << '\n';
			    retry = true;
                            goto stop;
			 }
                    } else if (boost::lexical_cast<double>(strArray[4]) != 0 && boost::lexical_cast<double>(strArray[4]) != statbufsrc.st_size) {
                        std::stringstream error_;
                        error_ << "User specified source file size is " << strArray[4] << " but stat returned " << statbufsrc.st_size;
                        errorMessage = error_.str();
                        log << fileManagement->timestamp() << "ERROR " << errorMessage << '\n';
                        errorScope = SOURCE;
                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                        errorPhase = TRANSFER_PREPARATION;
                        if (sourceStatRetry == 3){
			    log << fileManagement->timestamp() << "INFO No more retries for stat the source" << '\n';
			    retry = true;
                            goto stop;
			}
                    } else {
                        log << fileManagement->timestamp() << "INFO Source file size: " << statbufsrc.st_size << '\n';
                        if (statbufsrc.st_size > 0)
                            source_size = statbufsrc.st_size;
                        //conver longlong to string
                        std::string size_to_string = to_string<long double > (source_size, std::dec);
                        //set the value of file size to the message
                        msg_ifce::getInstance()->set_file_size(&tr_completed, size_to_string.c_str());
                        break;
                    }
                }
		log << fileManagement->timestamp() << "INFO Stat the source file will be retried" << '\n';
                sleep(3); //give it some time to breath
            }

            /*Checksuming*/
            if (compare_checksum) {
                if (checksum_value.length() > 0) { //user provided checksum
                    log << fileManagement->timestamp() << "INFO user  provided checksum" << '\n';
                    std::vector<std::string> token = split((strArray[3]).c_str());
                    std::string checkAlg = token[0];
                    std::string csk = token[1];
                    gfalt_set_user_defined_checksum(params, checkAlg.c_str(), csk.c_str(), NULL);
                    gfalt_set_checksum_check(params, TRUE, NULL);
                } else {//use auto checksum
                    log << fileManagement->timestamp() << "INFO Calculate checksum auto" << '\n';
                    gfalt_set_checksum_check(params, TRUE, NULL);
                }
            }

            //overwrite dest file if exists
            if (overwrite) {
                log << fileManagement->timestamp() << "INFO Overwrite is enabled" << '\n';
                gfalt_set_replace_existing_file(params, TRUE, NULL);
            }

            gfalt_set_timeout(params, timeout, NULL);
	
            unsigned int experimentalNstreams = adjustStreamsBasedOnSize(statbufsrc.st_size, nbstreams);
	    nbstreams = experimentalNstreams;
            gfalt_set_nbstreams(params, experimentalNstreams, NULL);
            nstream_to_string = to_string<unsigned int>(experimentalNstreams, std::dec);
            msg_ifce::getInstance()->set_number_of_streams(&tr_completed, nstream_to_string.c_str());

            gfalt_set_tcp_buffer_size(params, tcpbuffersize, NULL);
            gfalt_set_monitor_callback(params, &call_perf, NULL);

            //check all params before passed to gfal2
            if ((strArray[1]).c_str() == NULL || (strArray[2]).c_str() == NULL) {
   	         errorMessage = "Failed to get source or dest surl";
                 log << fileManagement->timestamp() << "ERROR " << errorMessage << '\n';
		 errorScope = TRANSFER;
                 reasonClass = GENERAL_FAILURE;
                 errorPhase = TRANSFER;
		 goto stop;
            }

            log << fileManagement->timestamp() << "INFO Transfer Starting" << '\n';
            if ((ret = gfalt_copy_file(handle, params, (strArray[1]).c_str(), (strArray[2]).c_str(), &tmp_err)) != 0) {
                if (tmp_err != NULL && tmp_err->message != NULL) {
                    log << fileManagement->timestamp() << "ERROR Transfer failed - errno: " << tmp_err->code << " Error message:" << tmp_err->message << '\n';
                    if (tmp_err->code == 110) {
                        errorMessage = std::string(tmp_err->message);
                        errorMessage += ", operation timeout";
                    } else {
                        errorMessage = std::string(tmp_err->message);
                    }
                    errorScope = TRANSFER;
                    reasonClass = mapErrnoToString(tmp_err->code);
                    errorPhase = TRANSFER;
                } else {
                    log << fileManagement->timestamp() << "ERROR Transfer failed - Error message: Unresolved error" << '\n';
                    errorMessage = std::string("Unresolved error");
                    errorScope = TRANSFER;
                    reasonClass = GENERAL_FAILURE;
                    errorPhase = TRANSFER;
                }
		retry = retryTransfer(tmp_err->code, "TRANSFER" );
                g_clear_error(&tmp_err);
                goto stop;
            } else {
                log << fileManagement->timestamp() << "INFO Transfer completed successfully" << '\n';
            }

            transferred_bytes = source_size;
            bytes_to_string = to_string<double>(transferred_bytes, std::dec);
            msg_ifce::getInstance()->set_total_bytes_transfered(&tr_completed, bytes_to_string.c_str());

            log << fileManagement->timestamp() << "INFO Stat the dest surl start" << '\n';
            for (int destStatRetry = 0; destStatRetry < 4; destStatRetry++) {
                if (gfal2_stat(handle, (strArray[2]).c_str(), &statbufdest, &tmp_err) < 0) {
                    if (tmp_err->message) {
                        std::string tempError(tmp_err->message);
                        log << fileManagement->timestamp() << "ERROR Failed to get dest file size, errno:" << tempError << '\n';
                        errorMessage = "Failed to get dest file size: " + tempError;
                        errorScope = DESTINATION;
                        reasonClass = mapErrnoToString(tmp_err->code);
                        errorPhase = TRANSFER_FINALIZATION;
                    } else {
                        std::string tempError = "Undetermined error";
                        log << fileManagement->timestamp() << "ERROR Failed to get dest file size, errno:" << tempError << '\n';
                        errorMessage = "Failed to get dest file size: " + tempError;
                        errorScope = DESTINATION;
                        reasonClass = mapErrnoToString(tmp_err->code);
                        errorPhase = TRANSFER_FINALIZATION;
                    }
                    retry = retryTransfer(tmp_err->code, "DESTINATION" );
                    if (destStatRetry == 3 || false == retry) {
                        log << fileManagement->timestamp() << "INFO No more retry stating the destination" << '\n';
			g_clear_error(&tmp_err);
                        goto stop;
                    }
		    g_clear_error(&tmp_err);
                } else {
                    if (statbufdest.st_size <= 0) {
                        errorMessage = "Destination file size is 0";
                        log << fileManagement->timestamp() << "ERROR " << errorMessage << '\n';
                        errorScope = DESTINATION;
                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                        errorPhase = TRANSFER_FINALIZATION;
                        if (destStatRetry == 3) {
                            log << fileManagement->timestamp() << "INFO No more retry stating the destination" << '\n';
			    retry = true;
                            goto stop;
                        }
                    } else if (boost::lexical_cast<double>(strArray[4]) != 0 && boost::lexical_cast<double>(strArray[4]) != statbufdest.st_size) {
                        std::stringstream error_;
                        error_ << "User specified destination file size is " << strArray[4] << " but stat returned " << statbufsrc.st_size;
                        errorMessage = error_.str();
                        log << fileManagement->timestamp() << "ERROR " << errorMessage << '\n';
                        errorScope = DESTINATION;
                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                        errorPhase = TRANSFER_FINALIZATION;
                        if (destStatRetry == 3) {
			    retry = true;
                            log << fileManagement->timestamp() << "INFO No more retry stating the destination" << '\n';
                            goto stop;
                        }
                    } else {
                        log << fileManagement->timestamp() << "INFO Destination file size: " << statbufdest.st_size << '\n';
                        dest_size = statbufdest.st_size;
                        break;
                    }
                }
    	        log << fileManagement->timestamp() << "WARN Stat the destination will be retried" << '\n';
                sleep(3); //give it some time to breath
            }

            //check source and dest file sizes
            if (source_size == dest_size) {
                log << fileManagement->timestamp() << "INFO Source and destination file size matching" << '\n';
            } else {
                log << fileManagement->timestamp() << "ERROR Source and destination file size are different" << '\n';
                errorMessage = "Source and destination file size mismatch";
                errorScope = DESTINATION;
                reasonClass = mapErrnoToString(gfal_posix_code_error());
                errorPhase = TRANSFER_FINALIZATION;
                goto stop;
            }

            gfalt_set_user_data(params, NULL, NULL);
        }//logStream
stop:       	
        diff = transferDuration(transfer_start, transfer_complete);  	
        msg_ifce::getInstance()->set_transfer_error_scope(&tr_completed, errorScope);
        msg_ifce::getInstance()->set_transfer_error_category(&tr_completed, reasonClass);
        msg_ifce::getInstance()->set_failure_phase(&tr_completed, errorPhase);
        msg_ifce::getInstance()->set_transfer_error_message(&tr_completed, errorMessage);
        if (errorMessage.length() > 0) {
            msg_ifce::getInstance()->set_final_transfer_state(&tr_completed, "Error");
            reporter.timeout = timeout;
            reporter.nostreams = nbstreams;
            reporter.buffersize = tcpbuffersize;            
            if (!terminalState) {
                logStream << fileManagement->timestamp() << "INFO Report FAILED back to the server" << '\n';
                reporter.constructMessage(retry, job_id, strArray[0], "FAILED", errorMessage, diff, source_size);
            }
        } else {
            msg_ifce::getInstance()->set_final_transfer_state(&tr_completed, "Ok");
            reporter.timeout = timeout;
            reporter.nostreams = nbstreams;
            reporter.buffersize = tcpbuffersize;
            logStream << fileManagement->timestamp() << "INFO Report FINISHED back to the server" << '\n';
            reporter.constructMessage(false, job_id, strArray[0], "FINISHED", errorMessage, diff, source_size);
	    /*unpin the file here and report the result in the log file...*/
 	     g_clear_error(&tmp_err);
	     if (bringonline > 0 && gfal2_release_file(handle, (strArray[1]).c_str(), (strArray[6]).c_str(), &tmp_err) < 0) {
	        logStream << fileManagement->timestamp() << "INFO Token: " << strArray[6] << '\n';
		if (tmp_err && tmp_err->message) {
                        logStream << fileManagement->timestamp() << "WARN Failed unpinning the file: " << std::string(tmp_err->message) << '\n';		
		}
	     }
        }
        logStream << fileManagement->timestamp() << "INFO Send monitoring complete message" << '\n';
        msg_ifce::getInstance()->set_tr_timestamp_complete(&tr_completed, msg_ifce::getInstance()->getTimestamp());
        msg_ifce::getInstance()->SendTransferFinishMessage(&tr_completed);
        logStream << fileManagement->timestamp() << "INFO Closing the log stream" << '\n';
        if (logStream.is_open()) {
            logStream.close();
        }
        if (debug == true) {
            fclose(stderr);
        }

        std::string moveFile = fileManagement->archive();        
    }//end for reuse loop

    if (params) {
        gfalt_params_handle_delete(params, NULL);
        params = NULL;
    }
    if (handle) {
        gfal_context_free(handle);
        handle = NULL;
    }

    if (cert) {
        delete cert;
        cert = NULL;
    }

    if (reuseFile.length() > 0)
        unlink(readFile.c_str());


    if (fileManagement)
        delete fileManagement;


    StaticSslLocking::kill_locks(); 
    
    return EXIT_SUCCESS;
}
