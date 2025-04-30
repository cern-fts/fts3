/*
 * Copyright (c) CERN 2024
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

#include <chrono>

/**
 * Return the current time as UNIX timestamp (in seconds).
 * An optional offset may be added to the returned value
 *
 * @param offset Add the offset to the returned timestamp value
 * @return current time as UNIX timestamp (in seconds)
 */
inline int64_t getTimestampSeconds(unsigned offset = 0)
{
    std::chrono::seconds timestamp =
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        );

    return timestamp.count() + offset;
}

/**
 * Return the current time as UNIX timestamp (in milliseconds).
 * An optional offset may be added to the returned value
 *
 * @param offset Add the offset to the returned timestamp value
 * @return current time as UNIX timestamp (in seconds)
 */
inline int64_t getTimestampMilliseconds(unsigned offset = 0)
{
    std::chrono::milliseconds timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        );

    return timestamp.count() + offset;
}
