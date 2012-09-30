#ifndef STATIC_SSL_LOCKING_H
#define STATIC_SSL_LOCKING_H

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <assert.h>

#include "threadabstractionlayer.h"
#include "threadtraits.h"

using namespace FTS3_COMMON_NAMESPACE;

/// static locking mechanism for OpenSSL
class StaticSslLocking
{

  public:

	/// constructor
	StaticSslLocking ( );

	/// destructor
	~StaticSslLocking ( );

	/// static mutex array
	static MUTEX_TYPE * poMutexes;
	static ThreadTraits::MUTEX _mutex; 
	
	static void init_locks();
	static void kill_locks();

  protected:

	/// callback for locking/unlocking a mutex in the array
	static void SslStaticLockCallback ( int inMode, 
										int inMutex,
										const char * ipsFile,
										int inLine );

	/// returns the id of the thread from which it is called
	static unsigned long SslThreadIdCallback ( );



};


#endif // STATIC_SSL_LOCKING_H
