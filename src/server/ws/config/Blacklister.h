/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BLACKLISTER_H_
#define BLACKLISTER_H_

#include <string>

#include <boost/optional.hpp>

#include "db/generic/SingleDbInstance.h"

#include "ws-ifce/gsoap/gsoap_stubs.h"

namespace fts3
{
namespace ws
{

using namespace std;
using namespace boost;
using namespace db;

class Blacklister
{

public:

    Blacklister(soap* ctx, string name, string status, int timeout, bool blk);

    Blacklister(soap* ctx, string name, string vo, string status, int timeout, bool blk);

    virtual ~Blacklister();

    void executeRequest();

private:

    void handleSeBlacklisting();
    void handleDnBlacklisting();

    void handleJobsInTheQueue();

    string adminDn;

    optional<string> vo;
    string name;
    string status;
    int timeout;
    bool blk;

    GenericDbIfce* db;

};

} /* namespace ws */
} /* namespace fts3 */
#endif /* BLACKLISTER_H_ */
