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

#include "StaticSslLocking.h"
#include <iostream>

struct CRYPTO_dynlock_value {
    MUTEX_TYPE mutex;
};

static MUTEX_TYPE *mutex_buf;

static struct CRYPTO_dynlock_value *dyn_create_function(const char *, int) {
    struct CRYPTO_dynlock_value *value;
    value = (struct CRYPTO_dynlock_value*) malloc(sizeof (struct CRYPTO_dynlock_value));
    if (value)
        MUTEX_SETUP(value->mutex);
    return value;
}

static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l, const char *, int) {
    if (mode & CRYPTO_LOCK)
        MUTEX_LOCK(l->mutex);
    else
        MUTEX_UNLOCK(l->mutex);
}

static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *, int) {
    MUTEX_CLEANUP(l->mutex);
    free(l);
}

static void locking_function(int mode, int n, const char *, int) {
    if (mode & CRYPTO_LOCK)
        MUTEX_LOCK(mutex_buf[n]);
    else
        MUTEX_UNLOCK(mutex_buf[n]);
}

static unsigned long id_function() {
    return (unsigned long) THREAD_ID;
}

static int CRYPTO_thread_setup() {
    int i;
    mutex_buf = (MUTEX_TYPE*) malloc(static_cast<size_t>(CRYPTO_num_locks()) * sizeof (pthread_mutex_t));
    if (!mutex_buf)
        return -1;
    for (i = 0; i < CRYPTO_num_locks(); i++)
        MUTEX_SETUP(mutex_buf[i]);
    CRYPTO_set_id_callback(id_function);
    CRYPTO_set_locking_callback(locking_function);
    CRYPTO_set_dynlock_create_callback(dyn_create_function);
    CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
    CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);
    return 0;
}

void CRYPTO_thread_cleanup() {
    int i;
    if (!mutex_buf)
        return;
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);
    CRYPTO_set_dynlock_create_callback(NULL);
    CRYPTO_set_dynlock_lock_callback(NULL);
    CRYPTO_set_dynlock_destroy_callback(NULL);
    for (i = 0; i < CRYPTO_num_locks(); i++)
        MUTEX_CLEANUP(mutex_buf[i]);
    free(mutex_buf);
    mutex_buf = NULL;
}


// deal with static variables
MUTEX_TYPE * StaticSslLocking::poMutexes;


void StaticSslLocking::SslStaticLockCallback(int inMode, int inMutex, const char * ipsFile, int inLine) {
    (void) ipsFile;
    (void) inLine;

    if (inMode & CRYPTO_LOCK) {
        MUTEX_LOCK(StaticSslLocking::poMutexes [ inMutex ]);
    } else {
        MUTEX_UNLOCK(StaticSslLocking::poMutexes [ inMutex ]);
    }

}

unsigned long StaticSslLocking::SslThreadIdCallback() {
    return ( (unsigned long) THREAD_ID);
}

void StaticSslLocking::init_locks() {

    CRYPTO_thread_setup();
    /* LEAVE IT HERE JUST FOR REFERENCE OF STATIC USAGE
            // If some other library already set these 
            if ( CRYPTO_get_locking_callback() && CRYPTO_get_id_callback() ) {
                    fprintf( stdout, "OpenSSL locking callbacks are already in place?\n" );
                    fflush( stdout );
                    //return ;
            }

            StaticSslLocking::poMutexes = new MUTEX_TYPE [ CRYPTO_num_locks ( ) ];	

            for ( int i = 0; i < CRYPTO_num_locks ( ); i++ ) {

                    MUTEX_SETUP ( StaticSslLocking::poMutexes [ i ] );

            }

            CRYPTO_set_id_callback ( StaticSslLocking::SslThreadIdCallback );
            CRYPTO_set_locking_callback ( StaticSslLocking::SslStaticLockCallback );	
     */
}

StaticSslLocking::StaticSslLocking() {
}

StaticSslLocking::~StaticSslLocking() {
}

void StaticSslLocking::kill_locks() {

    CRYPTO_thread_cleanup();
    /* LEAVE IT HERE JUST FOR REFERENCE OF STATIC USAGE
            if(StaticSslLocking::poMutexes)
                    return;

            CRYPTO_set_id_callback ( NULL );
            CRYPTO_set_locking_callback ( NULL );

            for ( int i = 0; i < CRYPTO_num_locks ( ); i++ ) {
                   if(StaticSslLocking::poMutexes) 
                    MUTEX_CLEANUP ( StaticSslLocking::poMutexes [ i ] );

            }

            if(StaticSslLocking::poMutexes){
                    delete [] StaticSslLocking::poMutexes;
                    StaticSslLocking::poMutexes = NULL;
            }
     */
}
