/*
 * FileTransferScheduler.h
 *
 *  Created on: May 7, 2012
 *      Author: simonm
 */

#ifndef FILETRANSFERSCHEDULER_H_
#define FILETRANSFERSCHEDULER_H_

#include <vector>
#include <string>

#include <boost/regex.hpp>

#include "db/generic/SingleDbInstance.h"

using namespace std;
using namespace boost;

class FileTransferScheduler {
public:
	FileTransferScheduler(TransferFiles* file);
	virtual ~FileTransferScheduler();

	bool schedule();

private:

	enum IO {
		INBOUND,
		OUTBOUND
	};

	enum Share {
		VO,
		PUBLIC,
		PAIR
	};

	struct Config {
		int inbound;
		int outbound;
		bool set;

		Config() {
			set = false;
		}

		operator bool() const {
			return set;
		}

		int operator[] (IO index) {

			if(index == INBOUND) {
				return inbound;
			}

			return outbound;
		}

	};

	int getCreditsInUse(IO io, Share share, const string type);

	int getFreeCredits(IO io, Share share, const string type, string name, string param);

	Config getValue (string type, string se, string shareId);

	inline string voShare(string vo) {
		return "\"share_type\":\"vo\",\"share_id\":\"" + vo + "\"";
	};

	inline string publicShare() {
		return "\"share_type\":\"public\",\"share_id\":null";
	};

	inline string pairShare(string pair) {
		return "\"share_type\":\"pair\",\"share_id\":\"" + pair + "\"";
	}

	int totalNumberOfCredits();

	regex re;
	smatch what;

	TransferFiles* file;
	string srcSeName;
	string destSeName;
	string voName;
	string srcSiteName;
	string destSiteName;

	static const int SE_NAME_REGEX_INDEX = 1;
	static const int INBOUND_REGEX_INDEX = 1;
	static const int OUTBOUND_REGEX_INDEX = 2;

	static const string SE_TYPE;
	static const string SITE_TYPE;

	static const string seNameRegex;
	static const string shareValueRegex;

	static const int DEFAULT_VALUE = 50;
	static const int FREE_CREDITS = 1;
};

#endif /* FILETRANSFERSCHEDULER_H_ */
