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


using namespace FTS3_COMMON_NAMESPACE;
using namespace std;
namespace po = boost::program_options;

int main(int argc, char **argv) {

    std::string job_id;
    std::string file_id; // (generated on server, passed to fts3_url_copy, NAME OF THE TR LOG FILE)	
    std::string source_url;
    std::string dest_url;
    //int overwrite; 
    int nbstreams;
    int tcpbuffersize;
    int blocksize;
    int timeout;
    //int daemonize;
    std::string checksum;
    std::string checksumAlgorithm;
    
    po::options_description desc("fts3_url_copy options");
    desc.add_options()
            ("help,h", "help")
            ("job_id", po::value<string > (&job_id), "job id")
            ("file_id", po::value<string > (&file_id), "file id")
            ("source_url", po::value<string > (&source_url), "source url")
            ("dest_url", po::value<string > (&dest_url), "dest url")
            ("overwrite", "overwrite")
            ("nbstreams", po::value<int>(&nbstreams)->default_value(5), "nbstreams")
            ("tcpbuffersize", po::value<int>(&tcpbuffersize)->default_value(0), "tcpbuffersize")
            ("blocksize", po::value<int>(&blocksize)->default_value(0), "blocksize")
            ("timeout", po::value<int>(&timeout)->default_value(3600), "timeout")
            ("daemonize", "daemonize")
            ("console", "console")
            ("blocking", "blocking")
            ;

    po::variables_map vm;
    // parse regular options
    po::positional_options_description p;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    


    // parse positional options
    po::store(po::command_line_parser(argc, argv). options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    if (argc == 1) {
	FTS3_COMMON_LOGGER_NEWLOG (ERR) << " Source url and Destination url are required" << commit;
        return EXIT_FAILURE;
    }

    if (vm.count("job_id")) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Job id: " << vm["job_id"].as<string > ()  << commit;	
    }

    if (vm.count("file_id")) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "File id: " << vm["file_id"].as<string > ()  << commit;		
    }

    if (vm.count("source_url")) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) <<  "Source url: " << vm["source_url"].as<string > ()  << commit;			
    } else {
        FTS3_COMMON_LOGGER_NEWLOG (ERR) << " Source url is required to be set" << commit;
        return EXIT_FAILURE;
    }

    if (vm.count("dest_url")) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Source url: " << vm["dest_url"].as<string > ()  << commit;				
    } else {
        FTS3_COMMON_LOGGER_NEWLOG (ERR) << " Destination url is required to be set" << commit;
        return EXIT_FAILURE;
    }

    if (vm.count("nbstreams")) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "nbstreams was set to " << vm["nbstreams"].as<int>() << commit;
    }

    if (vm.count("tcpbuffersize")) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "tcpbuffersize was set to " << vm["tcpbuffersize"].as<int>() << commit;	
    }

    if (vm.count("blocksize")) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "blocksize was set to " << vm["blocksize"].as<int>() << commit;		
	
    }

    if (vm.count("timeout")) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Timeout was set to " << vm["timeout"].as<int>() << commit;		    
    }

    if (vm.count("overwrite"))
    	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Overwrite was set" << commit;		           

    if (vm.count("console"))
    	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Print console was set" << commit;		               

    if (vm.count("blocking"))
    	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Blocking mode was set" << commit;		                           

    if (vm.count("daemonize")) {
    	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Daemonize was set" << commit;		                               
        daemon(0, 0);
    }

    FTS3_COMMON_LOGGER_NEWLOG (INFO) <<  " Transfer Accepted" << commit;
    //create log file named file_id.
    //IF file id doesn't exists (meaning fts3_url_copy direclty, create on with mktemp)


    //init message_queue
    QueueManager* qm = NULL;
    try {
        qm = new QueueManager(job_id, file_id);
    } catch (interprocess_exception &ex) {
        if (qm)
            delete qm;
        std::cout << ex.what() << std::endl;
    }

    //start transfer gfal2	
    GError * tmp_err = NULL; // classical GError/glib error management
    gfal_context_t handle;
    int ret = -1;
  
    // initialize gfal   
    if ((handle = gfal_context_new(&tmp_err)) == NULL) {
	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Transfer Initialization failed - errno: " <<  tmp_err->message << " Error message:" << tmp_err->message << commit;
        return -1;
    }
    // begin copy
        FTS3_COMMON_LOGGER_NEWLOG (INFO) <<  " Transfer Started" << commit;
    if ((ret = gfalt_copy_file(handle, NULL, source_url.c_str(), dest_url.c_str(), &tmp_err)) != 0) {        
	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Transfer failed - errno: " <<  tmp_err->message << " Error message:" << tmp_err->message << commit;
        return -1;
    } else
        FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Transfer completed successfully"  << commit;

    //release all resources
    gfal_context_free(handle);
    if (qm)
        delete qm;

    return EXIT_SUCCESS;
}

