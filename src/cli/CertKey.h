/*
 * Copyright (c) CERN 2013-2017
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

#ifndef FTS3_CERTKEY_H
#define FTS3_CERTKEY_H

#include <string>

namespace fts3 {
namespace cli {

struct CertKeyPair {
    std::string cert;
    std::string key;

    CertKeyPair() {}
    explicit CertKeyPair(std::string const &c): cert(c), key(c) {}
    CertKeyPair(std::string const &c, std::string const &k): cert(c), key(k) {}
};

}
}

#endif //FTS3_CERTKEY_H
