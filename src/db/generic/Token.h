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
    Token() :
        expiry(0)
    {}

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
    time_t expiry;

    /// Needed for STL container operations
    inline bool operator < (const Token& other) const
    {
        return tokenId < other.tokenId;
    }
};

/**
* Object representation of an exchanged token.
*/
class ExchangedToken
{
public:
    ExchangedToken() :
        expiry(0)
    {}

    ExchangedToken(const std::string& id, const std::string& access,
                   const std::string& refresh, const std::string& prevAccess, const time_t& exp) :
           tokenId(id), accessToken(access), refreshToken(refresh), previousAccessToken(prevAccess), expiry(exp)
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
    std::string previousAccessToken;
    time_t expiry;

    /// Needed for STL container operations
    inline bool operator < (const ExchangedToken& other) const
    {
        return tokenId < other.tokenId;
    }
};

/**
* Object representation of a refreshed access token.
*/
class RefreshedToken
{
public:
    RefreshedToken() :
        expiry(0)
    {}

    RefreshedToken(const std::string& id, const std::string& access,
                   const std::string& refresh, const std::string& prevRefresh, const time_t& exp) :
        tokenId(id), accessToken(access), refreshToken(refresh), previousRefreshToken(prevRefresh), expiry(exp)
    {}

    ~RefreshedToken() = default;

    std::string accessTokenToString() const {
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
    std::string previousRefreshToken;
    time_t expiry;

    /// Needed for STL container operations
    inline bool operator < (const RefreshedToken& other) const
    {
        return tokenId < other.tokenId;
    }
};
