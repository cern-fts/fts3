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

#include <gtest/gtest.h>

#include "db/generic/StorageConfig.h"

TEST(DbTest, SeConfigMerge) {
    StorageConfig target;
    target.inboundMaxActive = 60;

    StorageConfig src;
    src.udt = true;
    src.inboundMaxActive = 100;
    src.outboundMaxActive = 100;

    target.merge(src);

    EXPECT_EQ(60, target.inboundMaxActive);
    EXPECT_EQ(100, target.outboundMaxActive);
    EXPECT_EQ(true, target.udt.value);
    EXPECT_TRUE(boost::indeterminate(target.ipv6));
}
