/*
 * ActivityCfg.cpp
 *
 *  Created on: Oct 1, 2013
 *      Author: simonm
 */

#include "ActivityCfg.h"

namespace fts3 {
namespace ws {


ActivityCfg::ActivityCfg(string dn, string name) : Configuration(dn), vo(name)
{
	init(vo);
}

ActivityCfg::ActivityCfg(string dn, CfgParser& parser) : Configuration(dn)
{
    vo = parser.get<string>("vo");
    active = parser.get<bool>("active");
    shares = parser.get< map<string, double> >("share");

    all = json();
}

ActivityCfg::~ActivityCfg()
{

}

string ActivityCfg::json()
{

    stringstream ss;

    ss << "{";
    ss << "\"" << "vo" << "\":\"" << vo << "\",";
    ss << "\"" << "active" << "\":" << (active ? "true" : "false") << ",";
    ss << "\"" << "share" << "\":" << Configuration::json(shares);
    ss << "}";

    return ss.str();
}

void ActivityCfg::save()
{
	if (db->getActivityConfig(vo).empty()) {
		// insert
		db->addActivityConfig(vo, Configuration::json(shares), active);

	} else {
		// update
		db->updateActivityConfig(vo, Configuration::json(shares), active);
	}
}

void ActivityCfg::del()
{
	db->deleteActivityConfig(vo);
}

void ActivityCfg::init(string vo)
{
	active = db->isActivityConfigActive(vo);
	shares = db->getActivityConfig(vo);

	if (shares.empty())
		throw Err_Custom("There is no activity configuration for: " + vo);
}

} /* namespace ws */
} /* namespace fts3 */
