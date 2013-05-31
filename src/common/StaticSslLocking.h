/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

#ifndef STATIC_SSL_LOCKING_H
#define STATIC_SSL_LOCKING_H

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <assert.h>

#include "threadabstractionlayer.h"

/// static locking mechanism for OpenSSL

class StaticSslLocking
{
public:

    /// constructor
    StaticSslLocking();

    /// destructor
    ~StaticSslLocking();

    /// static mutex array
    static MUTEX_TYPE * poMutexes;

    static void init_locks();
    static void kill_locks();

protected:

    /// callback for locking/unlocking a mutex in the array
    static void SslStaticLockCallback(int inMode,
                                      int inMutex,
                                      const char * ipsFile,
                                      int inLine);

    /// returns the id of the thread from which it is called
    static unsigned long SslThreadIdCallback();



};


#endif // STATIC_SSL_LOCKING_H
