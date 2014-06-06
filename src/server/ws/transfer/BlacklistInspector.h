/*
 * BlacklistInspector.h
 *
 *  Created on: 27 May 2014
 *      Author: simonm
 */

#ifndef BLACKLISTINSPECTOR_H_
#define BLACKLISTINSPECTOR_H_

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <set>
#include <map>

namespace fts3 {
namespace ws {

using namespace db;

class BlacklistInspector
{
    class TimeoutAssigner
    {

    public:

        TimeoutAssigner(std::map<std::string, int>& timeouts) : timeouts(timeouts) {}

        template<typename T>
        void operator ()(T& t)
        {
            std::string const & src = t.source_se;
            std::string const & dst = t.dest_se;
            // if theres's nothing to do continue
            if (timeouts.find(src) == timeouts.end() && timeouts.find(dst) == timeouts.end()) return;
            // if src has no timeout use destination and continue
            if (timeouts.find(src) == timeouts.end())
                {
                    t.wait_timeout = timeouts[dst];
                    return;
                }
            // if dst has no time use source
            if (timeouts.find(dst) == timeouts.end())
                {
                    t.wait_timeout = timeouts[src];
                    return;
                }
            // if both dst and src have timeout pick the lower value
            t.wait_timeout = timeouts[src] < timeouts[dst] ? timeouts[src] : timeouts[dst];
        }

    private:

        std::map<std::string, int> timeouts;
    };

public:

	BlacklistInspector(std::string const & vo) : db (DBSingleton::instance().getDBObjectInstance()), vo(vo) {}
	virtual ~BlacklistInspector() {}

	void add(std::string const & se);

	void inspect() const;
	void setWaitTimeout(std::list<job_element_tupple> & jobs) const;

private:

    /// DB instance
    GenericDbIfce* db;

	std::set<std::string> unique_ses;
	std::string const & vo;
	std::string unique_ses_str;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* BLACKLISTINSPECTOR_H_ */
