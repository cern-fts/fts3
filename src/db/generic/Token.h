/*
 * Copyright (c) CERN 2023
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

#include <sstream>

/**
* Object representation of a token from the database.
*/
class Token
{
public:
    Token() = default;
    ~Token() = default;

    std::string getIssuer() const {
        if (issuer[issuer.size() - 1] != '/') {
            return issuer + "/";
        }

        return issuer;
    }

    std::string accessTokentoString() const {
        std::ostringstream out;
        out << accessToken.substr(0, 5) << "..."
            << accessToken.substr(accessToken.size() - 5);
        return out.str();
    }

    std::string tokenId;
    std::string accessToken;
    std::string refreshToken;
    std::string issuer;
};
