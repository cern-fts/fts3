/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License. 
You may obtain a copy of the License at 

    http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and 
limitations under the License. */

#pragma once

/** \file traced.h Interface of Traced class. */

#include "common_dev.h"
#include "common/logger.h"
#include <sstream>
#include <string>

FTS3_COMMON_NAMESPACE_START

/* -------------------------------------------------------------------------- */

/** A Traced class provides traceability and object identification for the class
 * specified in the template type parameter. This class must be inherited in this way:
 * 
 * class ConcreteTracedClass : public Traced<ConcreteTracedClass> ...
 * 
 * Construction and destruction of traced classes are logged. It also provides an
 * object identification object (string) for each traced objects.
 * 
 * The structure of the identification: 
 * 
 * <user-given class-wide prefix><system-generated class-wide unique postfix>
 * 
 * The system does not check if two different classes have different user-given
 * prefixes. To avoid confusion, the user must ensure it.
 *  
 * The class ensures that objects created in different threads have different id-s
 * in the whole application by a global, synchronized postfix generator. The postfixes
 * are simple integer numbers. 
 */
class TracedBase {
public:
	TracedBase() {};
	TracedBase(const TracedBase&) {};
	virtual ~TracedBase() {}; 
	/* type of the object id */
	typedef std::string id_type; 
	virtual const id_type& id() const = 0;
};

template <class TYPE, class POSTFIX_TYPE = std::string>
class Traced : public TracedBase
{
public: 
	/** Create an unique ID for the object. */
	Traced(const char* classPrefix, POSTFIX_TYPE postfix = POSTFIX_TYPE()) {		
		std::stringstream s;
		s << _classPrefix(classPrefix) << postfix;
		_id = s.str();
		FTS3_COMMON_LOGGER_NEWLOG(INFO) << id() << " created" << commit;
	}
	
	Traced(const Traced& t) : TracedBase(), _id(t._id) {}	
	
	virtual ~Traced() {
		FTS3_COMMON_LOGGER_NEWLOG(INFO) << id() << " deleted" << commit;
	}
	
	/** Return the object ID. */
	virtual const id_type& id() const {
		return _id;
	}
	
private:
	/** Store the class prefix string */
	static const std::string& _classPrefix (const char* first = "") {
		static std::string cp(first + std::string(":"));
		return cp;
	}
	
	/* The ID */
	id_type _id;
};

FTS3_COMMON_NAMESPACE_END

