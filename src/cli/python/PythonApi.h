/*
 * PythonCliWrapper.h
 *
 *  Created on: Sep 11, 2012
 *      Author: simonm
 */

#ifndef PYTHONCLIWRAPPER_H_
#define PYTHONCLIWRAPPER_H_

#include "GSoapContextAdapter.h"
#include <boost/python.hpp>



namespace fts3 { namespace cli {

using namespace boost::python;

class PythonApi {

public:
	PythonApi(str endpoint);
	virtual ~PythonApi();

	str submit(list elements, dict parameters, bool checksum); // deleg only
	void cancel(list ids);
	str getStatus(str id);

private:
	GSoapContextAdapter ctx;

	static const object none;

};

}
}

#endif /* PYTHONCLIWRAPPER_H_ */
