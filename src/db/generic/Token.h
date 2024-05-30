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

    std::string accessTokenToString() const {
        if (accessToken.empty()) {
            return "null";
        }

        std::ostringstream out;
        out << accessToken.substr(0, 5) << "..."
            << accessToken.substr(accessToken.size() - 5);
        return out.str();
    }

    std::string refreshTokenToString() const {
        if (refreshToken.empty()) {
            return "null";
        }

        std::ostringstream out;
        out << refreshToken.substr(0, 5) << "..."
            << refreshToken.substr(refreshToken.size() - 5);
        return out.str();
    }

    std::string tokenId;
    std::string accessToken;
    std::string refreshToken;
    std::string issuer;
    std::string scope;
    std::string audience;
};

/**
* Object representation of an exchanged token.
*/
class ExchangedToken
{
public:
    ExchangedToken() = default;

    ExchangedToken(const std::string& id, const std::string& access,
                   const std::string& refresh, const std::string& prevAccess) :
           tokenId(id), accessToken(access), refreshToken(refresh), previousAccessToken(prevAccess)
    {}

    ~ExchangedToken() = default;

    std::string accessTokenToString() const {
        if (accessToken.empty()) {
            return "null";
        }

        std::ostringstream out;
        out << accessToken.substr(0, 5) << "..."
            << accessToken.substr(accessToken.size() - 5);
        return out.str();
    }

    std::string tokenId;
    std::string accessToken;
    std::string refreshToken;
    std::string previousAccessToken;

    /// Needed for STL container operations
    inline bool operator < (const ExchangedToken& other) const
    {
        return tokenId < other.tokenId;
    }
};
