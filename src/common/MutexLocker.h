/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***********************************************/

/**
 * @file MutexLocker.h
 * @brief internal synch mechanishm
 * @author Michail Salichos
 * @date 09/02/2012
 *
 **/



#pragma once

#pragma once
#include <pthread.h>

class Mutex {
public:

    Mutex() {
        pthread_mutex_init(&m, 0);
    }

    void lock() {
        pthread_mutex_lock(&m);
    }

    void unlock() {
        pthread_mutex_unlock(&m);
    }

private:

    pthread_mutex_t m;

};

class MutexLocker {
public:

    MutexLocker(Mutex & pm)
    : m(pm) {
        m.lock();
    }

    ~MutexLocker() {
        m.unlock();
    }

private:

    Mutex & m;
};


