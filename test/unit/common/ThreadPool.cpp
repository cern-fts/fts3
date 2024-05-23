/*
 * Copyright (c) CERN 2024
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

#include <gtest/gtest.h>

#include "common/ThreadPool.h"

using fts3::common::ThreadPool;

struct EmptyTask {
    void run(boost::any const &) {}
};

TEST(ThreadPoolTest, ThreadPoolSize) {
    ThreadPool<EmptyTask> tp(10);
    EXPECT_EQ(tp.size(), 10);
    tp.join();
}

struct IdTask {
    IdTask(boost::thread::id &id) : id(id) {}

    void run(boost::any const &) {
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        id = boost::this_thread::get_id();
    }

    boost::thread::id &id;
};

TEST(ThreadPoolTest, ThreadPoolStart) {
    boost::thread::id ids[3];

    ThreadPool<IdTask> one_thread(1);
    one_thread.start(new IdTask(ids[0]));
    one_thread.start(new IdTask(ids[1]));
    one_thread.join();

    EXPECT_EQ(ids[0], ids[1]);

    ThreadPool<IdTask> two_threads(2);
    two_threads.start(new IdTask(ids[0]));
    two_threads.start(new IdTask(ids[1]));
    two_threads.join();

    EXPECT_NE(ids[0], ids[1]);

    ThreadPool<IdTask> three_tasks(2);
    three_tasks.start(new IdTask(ids[0]));
    three_tasks.start(new IdTask(ids[1]));
    three_tasks.start(new IdTask(ids[2]));
    three_tasks.join();

    EXPECT_NE(ids[0], ids[1]);
    EXPECT_TRUE(ids[2] == ids[0] || ids[2] == ids[1]);
}

struct SleepyTask {
    SleepyTask(bool &done) : done(done) {}

    void run(boost::any const &) {
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        done = true;
    }

    bool &done;
};

TEST(ThreadPoolTest, ThreadPoolJoin) {
    bool done = false;

    ThreadPool<SleepyTask> tp(1);
    tp.start(new SleepyTask(done));
    tp.join();
    EXPECT_TRUE(done);
}

struct InfiniteTask {
    void run(boost::any const &) {
        while (true) {
            boost::this_thread::interruption_point();
        }
    }
};

TEST(ThreadPoolTest, ThreadPoolInterrupt) {
    ThreadPool<InfiniteTask> tp(1);
    tp.start(new InfiniteTask());
    tp.interrupt();
    tp.join();
}

struct InitTask {
    InitTask(std::string &str) : str(str) {}

    void run(boost::any const &data) {
        if (data.empty()) return;
        std::string d = boost::any_cast<std::string>(data);
        str += d;
    }

    std::string &str;
};

void init_func(boost::any &data) {
    data = std::string(".00$");
}

TEST(ThreadPoolTest, ThreadPoolInitFunc) {
    using namespace fts3::common;

    std::string ret[2] = {"10", "100"};

    ThreadPool<InitTask> tp(2, init_func);

    tp.start(new InitTask(ret[0]));
    tp.start(new InitTask(ret[1]));
    tp.join();

    EXPECT_EQ(ret[0], "10.00$");
    EXPECT_EQ(ret[1], "100.00$");
}

struct InitCallableObject {
    void operator()(boost::any &data) {
        data = std::string(".00$");
    }
};


TEST(ThreadPoolTest, ThreadPoolInitObj) {
    using namespace fts3::common;

    std::string ret[2] = {"10", "100"};

    InitCallableObject obj;

    ThreadPool<InitTask, InitCallableObject> tp(2, obj);

    tp.start(new InitTask(ret[0]));
    tp.start(new InitTask(ret[1]));
    tp.join();

    EXPECT_EQ(ret[0], "10.00$");
    EXPECT_EQ(ret[1], "100.00$");
}


void zero_func(boost::any &data) {
    data = 0;
}


struct IncTask {
    void run(boost::any &data) {
        if (data.empty()) return;

        int &i = boost::any_cast<int &>(data);
        ++i;
    }
};


TEST(ThreadPoolTest, ThreadPoolReduce) {
    using namespace fts3::common;

    ThreadPool<IncTask> tp(2, zero_func);
    tp.start(new IncTask);
    tp.start(new IncTask);
    tp.start(new IncTask);
    tp.start(new IncTask);
    tp.join();

    int result = tp.reduce(std::plus<int>());
    EXPECT_EQ(result, 4);

    result = tp.reduce(std::minus<int>());
    EXPECT_EQ(result, -4);

    ThreadPool<IncTask> tp2(2, zero_func);
    tp2.start(new IncTask);
    tp2.join();
    result = tp2.reduce(std::plus<int>());
    EXPECT_EQ(result, 1);
}
