/*
 * OsgResourceNameFinder.h
 *
 *  Created on: Oct 22, 2012
 *      Author: simonm
 */

#ifndef OSGRESOURCENAMEFINDER_H_
#define OSGRESOURCENAMEFINDER_H_

#include <pugixml.hpp>
#include <string>

namespace fts3 { namespace common {

using namespace std;
using namespace pugi;

class OsgResourceNameFinder {

public:
	OsgResourceNameFinder(string path);
	virtual ~OsgResourceNameFinder();

	string getName(string fqdn);

private:

	xml_document doc;

	static string xpath_fqdn(string fqdn);
	static string xpath_fqdn_alias(string alias);
};

}
}

#endif /* OSGRESOURCENAMEFINDER_H_ */
