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

#pragma once
#ifndef PAIR_H
#define PAIR_H

#include <iostream>
#include <string>
#include "common/Uri.h"

struct Pair {
    std::string source, destination;

    Pair(const std::string &s, const std::string &d): source(s), destination(d) {
    }

    bool isLanTransfer() const {
        return fts3::common::isLanTransfer(source, destination);
    }
};

// Required so it can be used as a key on a std::map
inline bool operator < (const Pair &a, const Pair &b) {
    return a.source < b.source || (a.source == b.source && a.destination < b.destination);
}

inline std::ostream& operator << (std::ostream &os, const Pair &pair) {
    return (os << pair.source << " => " << pair.destination);
}


#endif // PAIR_H
