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
 *
 *
 * ZipCompressor.h
 *
 *  Created on: Oct 23, 2012
 *      Author: simonm
 */

#ifndef ZIPCOMPRESSOR_H_
#define ZIPCOMPRESSOR_H_

#include <iostream>
#include <string>

namespace fts3 { namespace common {

using namespace std;

class ZipCompressor {

public:
	ZipCompressor(istream& in, ostream& out): out(out),in(in) {}
	virtual ~ZipCompressor(){}

	void compress();

private:
	ostream& out; // a binary file e.g.
	istream& in;
};

} /* namespace common */ } /* namespace fts3 */
#endif /* ZIPCOMPRESSOR_H_ */
