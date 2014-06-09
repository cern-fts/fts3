/*
 * BlacklistInspector.cpp
 *
 *  Created on: 27 May 2014
 *      Author: simonm
 */

#include "BlacklistInspector.h"

#include "common/error.h"

#include <list>
#include <algorithm>
#include <numeric>

#include <boost/lambda/lambda.hpp>

namespace fts3 {
namespace ws {

using namespace fts3::common;

void BlacklistInspector::add(std::string const & se)
{
	// treat the first one differently
	if (unique_ses.empty())
		{
			unique_ses.insert(se);
			// add the parenthesis
			unique_ses_str += "('" + se + "')";
		}
	else if (!unique_ses.count(se))
        {
            unique_ses.insert(se);
            // add any subsequent SE before the closing parenthesis
            unique_ses_str.insert(unique_ses_str.size() - 1, ",'" + se + "'");
        }
}

void BlacklistInspector::inspect() const
{
	// get the list of SEs that are blacklisted
    std::list<std::string> notAllowed;
    db->allowSubmit(unique_ses_str, vo, notAllowed);

    // if all the SEs are OK there's nothing to do ...
    if (notAllowed.empty()) return;

    // accumulate all the blacklisted SEs in one string, comma seperated
    std::string notAllowedStr = std::accumulate(
    		notAllowed.begin(), notAllowed.end(), std::string(),
    		boost::lambda::_1 + boost::lambda::_2 + ","
    	);
    // erase the leading comma
    notAllowedStr.resize(notAllowedStr.size() - 1);

    // throw an exception ...
    throw Err_Custom("Following SEs: " + notAllowedStr + " are blacklisted!");
}

void BlacklistInspector::setWaitTimeout(std::list<job_element_tupple> & jobs) const
{
	// get the timeouts from DB
    std::map<std::string, int> timeouts;
    db->getTimeoutForSe(unique_ses_str, timeouts);
    // create the assigner object
    TimeoutAssigner assign_timeout(timeouts);
    // fill in wait timeouts
    std::for_each(jobs.begin(), jobs.end(), assign_timeout);
}

} /* namespace cli */
} /* namespace fts3 */
