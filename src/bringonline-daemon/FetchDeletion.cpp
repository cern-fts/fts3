/*
 * FetchDeletion.cpp
 *
 *  Created on: 15 Jul 2014
 *      Author: roiser
 */

#include "FetchDeletion.h"

#include "DeletionTask.h"
//#include "WaitingRoom.h"
#include "DeletionContext.h"

#include "server/DrainMode.h"
#include "cred/cred-utility.h"
#include "db/generic/SingleDbInstance.h"
#include "common/parse_url.h"

#include <map>

extern bool stopThreads;


void FetchDeletion::fetch()
{
  //WaitingRoom<PollTask>::instance().attach(threadpool);
  
  while (!stopThreads)
    {
      try  //this loop must never exit
	{
	  //if we drain a host, stop with deletions
	  if (DrainMode::getInstance())
	    {
	      FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, stopped deleting files with this instance!" << commit;
	      boost::this_thread::sleep(boost::posix_time::milliseconds(15000));
	      continue;
	    }
	  
	  std::map<key_type, DeletionContext> tasks;
	  
	  std::vector<DeletionContext::context_type> files;
	  db::DBSingleton::instance().getDBObjectInstance()->getFilesForDeletion(files);
	  
	  std::vector<DeletionContext::context_type>::iterator it_f;
	  for (it_f = files.begin(); it_f != files.end(); ++it_f)
	    {
	      // make sure it is a srm SE
	      std::string & url = boost::get<DeletionContext::source_url>(*it_f);
	      // get the SE name
	      Uri uri = Uri::Parse(url);
	      std::string se = uri.Host;
	      // get the other values necessary for the key
	      std::string & dn = boost::get<DeletionContext::user_dn>(*it_f);
	      std::string & vo = boost::get<DeletionContext::vo_name>(*it_f);
	      
	      tasks[key_type(vo, dn, se)].add(*it_f);
	    }
	  
	  std::map<key_type, DeletionContext>::const_iterator it_t;
	  for (it_t = tasks.begin(); it_t != tasks.end(); ++it_t)
	    {
	      try
		{
		  threadpool.start(new DeletionTask(*it_t));
		}
	      catch(Err_Custom const & ex)
		{
		  FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
		}
	      catch(...)
		{
		  FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
		}
	    }
	  
	  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
      catch (Err& e)
	{
	  FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION " << e.what() << commit;
	}
      catch (...)
	{
	  FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION Fatal error (unknown origin)" << commit;
	}
    }
}
