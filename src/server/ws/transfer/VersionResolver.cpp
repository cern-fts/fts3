/*
 * VersionResolver.cpp
 *
 *  Created on: Dec 14, 2012
 *      Author: simonm
 */

#include "VersionResolver.h"

#include <stdio.h>

#include <sstream>

namespace fts3 {
namespace ws {

VersionResolver::VersionResolver() {

    FILE *in;
    char buff[512];

    in = popen("rpm -q --qf '%{VERSION}' fts-server", "r");

    stringstream ss;
    while (fgets(buff, sizeof (buff), in) != NULL) {
        ss << buff;
    }

    pclose(in);

    version = ss.str();
    interface = version;
    schema = version; // TODO check DB type
    metadata = version; // TODO package name!
}

VersionResolver::~VersionResolver() {

}

string VersionResolver::getVersion() {
	return version;
}

string VersionResolver::getInterface() {
	return interface;
}

string VersionResolver::getSchema() {
	return schema;
}

string VersionResolver::getMetadata() {
	return metadata;
}

} /* namespace server */
} /* namespace fts3 */
