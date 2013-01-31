/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

/*
 * UuidGenerator.cpp
 *
 *  Created on: Feb 17, 2012
 *      Author: simonm
 */

#include "uuid_generator.h"
#include <uuid/uuid.h>


string UuidGenerator::generateUUID() {

	uuid_t id;
	char c_str[37]={0};

	uuid_generate(id);
	// different algorithms:
	//uuid_generate_random(id);
	//uuid_generate_time(id);
	uuid_unparse(id, c_str);

	string str = c_str;
	return str;
}
