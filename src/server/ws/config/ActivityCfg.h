/*
 * ActivityCfg.h
 *
 *  Created on: Oct 1, 2013
 *      Author: simonm
 */

#ifndef ACTIVITYCFG_H_
#define ACTIVITYCFG_H_

#include "Configuration.h"

#include <string>
#include <map>

namespace fts3
{
namespace ws
{



class ActivityCfg  : public Configuration
{
public:

    ActivityCfg(string dn, string name);

    ActivityCfg(string dn, CfgParser& parser);

    virtual ~ActivityCfg();

    /**
     * Creates a string containing the JSON configuration common for all 'standalone' configurations
     */
    virtual string json();

    /**
     * Saves the current configuration into the DB
     */
    virtual void save();

    /**
     * Removes the configuration from the DB
     */
    virtual void del();

    /**
     * Checks if the configuration concerns a single SE or a group
     */
    virtual bool isgroup()
    {
        return false;
    }

private:

    /**
     * Initializes the object from DB
     */
    void init(string vo);

    string vo;
    map<string, double> shares;
    bool active;

};

} /* namespace server */
} /* namespace fts3 */
#endif /* ACTIVITYCFG_H_ */
