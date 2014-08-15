/*
 * DeletionTask.cpp
 *
 *  Created on: 17 July 2014
 *      Author: roiser
 */

#include "DeletionTask.h"
#include "WaitingRoom.h"

#include "common/logger.h"
#include "common/error.h"

DeletionTask::DeletionTask(std::pair<key_type, DeletionContext> const & ctx) 
{
  // set up the gfal2 context
  GError *error = NULL;
  
  const char *protocols[] = {"rfio", "gsidcap", "dcap", "gsiftp"};
  
  gfal2_set_opt_string_list(gfal2_ctx, "SRM PLUGIN", "TURL_PROTOCOLS", protocols, 4, &error);
  if (error) {
    std::stringstream ss;
    ss << "BRINGONLINE Could not set the protocol list " << error->code << " " << error->message;
    throw Err_Custom(ss.str());
  }
  
  gfal2_set_opt_boolean(gfal2_ctx, "GRIDFTP PLUGIN", "SESSION_REUSE", true, &error);
  if (error) {
    std::stringstream ss;
    ss << "BRINGONLINE Could not set the session reuse " << error->code << " " << error->message;
    throw Err_Custom(ss.str());
  }
  
  // set the proxy certificate
  setProxy();
}

void DeletionTask::setProxy()
{
  GError *error = NULL;
  
  //before any operation, check if the proxy is valid
  std::string message;
  bool isValid = DeletionContext::checkValidProxy(ctx.proxy, message);
  if(!isValid) {
    //state_update(ctx.jobs, "FAILED", message, false);
    std::stringstream ss;
    ss << "DELETION proxy certificate not valid: " << message;
    throw Err_Custom(ss.str());
  }
  
  char* cert = const_cast<char*>(ctx.proxy.c_str());

    int status = gfal2_set_opt_string(gfal2_ctx, "X509", "CERT", cert, &error);
  if (status < 0) {
    //state_update(ctx.jobs, "FAILED", error->message, false);
    std::stringstream ss;
    ss << "DELETION setting X509 CERT failed " << error->code << " " << error->message;
    throw Err_Custom(ss.str());
  }
  
  status = gfal2_set_opt_string(gfal2_ctx, "X509", "KEY", cert, &error);
  if (status < 0) {
    //state_update(ctx.jobs, "FAILED", error->message, false);
    std::stringstream ss;
    ss << "DELETION setting X509 KEY failed " << error->code << " " << error->message;
    throw Err_Custom(ss.str());
  }
}

void DeletionTask::run(boost::any const &)
{
  GError *error = NULL;
  char token[512] = {0};
  
  std::vector<char const *> urls = ctx.getUrls();
  int status = gfal2_unlink_list(
			         gfal2_ctx,
			         urls.size(),
			         &*urls.begin(),
				 &error
				);

  if (status < 0) {
    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION FAILED "
				   << error->code << " " << error->message  << commit;
    
    //bool retry = retryTransfer(error->code, "SOURCE", std::string(error->message));
    //state_update(ctx.jobs, "FAILED", error->message, retry);
    g_clear_error(&error);
  }
  else {
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE FINISHED, got token " << token   << " "
                                    <<  commit;
    // No need to poll in this case!
    //state_update(ctx.jobs, "FINISHED", "", false);
  }
}

