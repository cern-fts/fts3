#ifndef THREAD_ABSTRACTION_LAYER
#define THREAD_ABSTRACTION_LAYER


/** 
 *  * defines a set of common thread macros between windows and posix
 *   */

#include <pthread.h>
#define MUTEX_TYPE          pthread_mutex_t
#define MUTEX_SETUP(x)      pthread_mutex_init ( &(x), NULL )
#define MUTEX_CLEANUP(x)    pthread_mutex_destroy ( &(x) )

#if VERBOSE_LOCKING

#define MUTEX_LOCK(x)       fprintf ( stdout, "[%d] trying to lock mutex %x %s:%d\n", pthread_self ( ), &x, __FILE__, __LINE__ ); pthread_mutex_lock ( &(x) ); fprintf ( stdout, "[%d] successfully locked mutex %x %s:%d\n", pthread_self ( ), &x, __FILE__, __LINE__ );
#define MUTEX_UNLOCK(x)     fprintf ( stdout, "[%d] unlocking mutex %x %s:%d\n", pthread_self ( ), &x,__FILE__, __LINE__ ); pthread_mutex_unlock ( &(x) )

#else

#define MUTEX_LOCK(x)       pthread_mutex_lock ( &(x) )
#define MUTEX_UNLOCK(x)     pthread_mutex_unlock ( &(x) )

#endif // VERBOSE_LOCKING

#define THREAD_ID           pthread_self ( )
#define THREAD_CC           __cdecl
#define THREAD_TYPE         pthread_t
#define THREAD_create(id,function,parameter) pthread_create ( &(id), NULL, (function), ((void *)(parameter)) )


#endif // THREAD_ABSTRACTION_LAYER
