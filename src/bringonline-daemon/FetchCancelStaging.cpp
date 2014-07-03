/*
 * FetchCancelStaging.cpp
 *
 *  Created on: 3 Jul 2014
 *      Author: simonm
 */

#include "FetchCancelStaging.h"

#include "CancelStagingTask.h"

#include "server/DrainMode.h"

#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include "common/error.h"

#include <boost/tuple/tuple.hpp>

#include <vector>
#include <string>

extern bool stopThreads;

void FetchCancelStaging::fetch()
{

	while(!stopThreads)
	{
        try  //this loop must never exit
        {
			//if we drain a host, no need to check if url_copy are reporting being alive
			if (DrainMode::getInstance())
			{
				FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more checking stage-in files for this instance!" << commit;
				boost::this_thread::sleep(boost::posix_time::milliseconds(15000));
				continue;
			}

			std::vector< boost::tuple<int, std::string, std::string> > files;
			db::DBSingleton::instance().getDBObjectInstance()->getStagingFilesForCanceling(files);

			std::vector< boost::tuple<int, std::string, std::string> >::const_iterator it;
			for (it = files.begin(); it != files.end(); ++it)
			{
                try
                    {
                        threadpool.start(new CancelStagingTask(*it));
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
			FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << e.what() << commit;
		}
		catch (...)
		{
			FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Fatal error (unknown origin)" << commit;
		}
	}
}
