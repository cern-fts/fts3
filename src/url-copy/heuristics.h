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

#ifndef HEURISTICS_H
#define HEURISTICS_H

#include <cstdint>
#include <string>

/**
 * Return true if the error is considered recoverable
 * (i.e. ENOENT is not recoverable)
 */
bool retryTransfer(int errorNo, const std::string &category, const std::string &message);

/**
 * Return a reasonable timeout for the given filesize
 */
unsigned adjustTimeoutBasedOnSize(uint64_t sizeInBytes, unsigned addSecPerMb);

std::string mapErrnoToString(int err);

std::string replaceMetadataString(const std::string &text);

/**
 * FTS-1995: Filter out anything that might be query string.
 * This is brute-force attempt in lack of sophisticated filtering.
 * The query string will be replaced with the "<redacted>" text
 */
std::string sanitizeQueryString(const std::string& text);

/**
 * Given a JWT access token, extract the required field
 * @param token the JWT access token
 * @param field the field to extract from the access token
 * @return the required field or empty string
 */
std::string extractAccessTokenField(const std::string& token, const std::string& field);

/**
 * Given a JWT access token, decode the payload
 * and print a list of relevant fields:
 * ["aud", "iss", "exp", "scope","wlcg.ver"]
 */
std::string accessTokenPayload(const std::string& token);

#endif // HEURISTICS_H
