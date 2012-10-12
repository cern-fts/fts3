/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * LogFileStreamer.h
 *
 *  Created on: Oct 12, 2012
 *      Author: Michał Simon
 */

#ifndef LOGFILESTREAMER_H_
#define LOGFILESTREAMER_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"
#include <string>
#include <fstream>

namespace fts3 { namespace ws {

using namespace std;

class LogFileStreamer {

public:
	static void readClose(soap* ctx, void* handle);

	static void* readOpen(soap* ctx, void* handle, const char* id, const char* type, const char* description);

	static size_t read(soap* ctx, void* handle, char* buff, size_t len);
};

}
}

#endif /* LOGFILESTREAMER_H_ */
