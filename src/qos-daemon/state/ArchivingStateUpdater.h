/*
 * Copyright (c) CERN 2013-2019
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#ifndef ARCHIVINGSTATEUPDATER_H_
#define ARCHIVINGSTATEUPDATER_H_

#include <string>
#include <vector>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include "db/generic/SingleDbInstance.h"
#include "common/Logger.h"

#include "StateUpdater.h"

using namespace fts3::common;

/**
 * A utility for carrying out asynchronous state and timestamp updates,
 * which are accumulated and than send to DB at the same time
 */
class ArchivingStateUpdater : public StateUpdater
{
public:

	// this is necessary because otherwise the operator would be hidden by the following one
	using StateUpdater::operator();


	/**
	 * Updates the Archiving start time for the given jobs/files
	 *
	 * @param jobs : jobs with respective files
	 */
	void operator()(const std::map<std::string, std::map<std::string, std::vector<uint64_t> > > &jobs)
	{
		try {
			db.setArchivingStartTime(jobs);
		}
		catch (std::exception& ex) {
			FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
		}
		catch(...) {
			FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception while setting the archiving start time!" << commit;
		}
	}

	/**
	 * Updates status per file
	 */
	void operator()(const std::string &jobId, uint64_t fileId, const std::string &state, const JobError &error)
	{
		// lock the vector
		boost::mutex::scoped_lock lock(m);
		updates.emplace_back(jobId, fileId, state, error.String(), error.IsRecoverable());
		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "ARCHIVING Update : "
				<< fileId << "  " << state << "  " << error.String() << " " << jobId << " " << error.IsRecoverable() << commit;
	}


	/// Destructor
	virtual ~ArchivingStateUpdater() {}

	using StateUpdater::recover;

private:
	friend class QoSServer;

	/// Default constructor
	ArchivingStateUpdater() : StateUpdater("_archiving") {}

	/// Copy constructor
	ArchivingStateUpdater(ArchivingStateUpdater const &) = delete;
	/// Assignment operator
	ArchivingStateUpdater & operator=(ArchivingStateUpdater const &) = delete;

	void run()
	{
		runImpl(&GenericDbIfce::updateArchivingState);
	}

	//TODO check if this is needed
	void recover(const std::vector<MinFileStatus> &recover)
	{
		if (!recover.empty()) {
			// lock the vector
			boost::mutex::scoped_lock lock(m);
			// put the items back
			updates.insert(updates.end(), recover.begin(), recover.end());
		}

		fts3::events::MessageBringonline msg;
		for (auto itFind = recover.begin(); itFind != recover.end(); ++itFind)
		{
			msg.set_file_id(itFind->fileId);
			msg.set_job_id(itFind->jobId);
			msg.set_transfer_status(itFind->state);
			msg.set_transfer_message(itFind->reason);

			//store the states into fs to be restored in the next run
			producer.runProducerStaging(msg);
		}
	}
};

#endif // ARCHIVINGSTATEUPDATER_H_
