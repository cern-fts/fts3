/*
 * ThreadContext.h
 *
 *  Created on: 25 Jun 2014
 *      Author: simonm
 */

#ifndef ThreadContext_H_
#define ThreadContext_H_

#include <sstream>
#include <string>

#include <boost/any.hpp>

#include <gfal_api.h>

/**
 * DmTask context
 *
 * 'init_context' routine should be passed to ThreadPool<DmTask>
 * constructor in order to create a gfal2 context for each thread
 */
class ThreadContext
{
	/**
	 * Prototype object used for creating ThreadContext
	 */
	struct ContextPrototype
	{
		/// name of the infosys in use
		std::string infosys;
	};

public:

	/**
	 * Creates a prototype for each ThreadContext
	 *
	 * @param infosys : infosys in use
	 */
	static void createContextPrototype(std::string const & infosys)
	{
		prototype.infosys = infosys;
	}

	/**
	 * Constructs a prototype ThreadContext based on prototype object
	 *
	 * It does not actually create the gfal2 context, it is just a wrapper
	 * around prototype so boost::any is assigned with a right object (ThreadContext)
	 *
	 * @param prototype : prototype object created using 'createContextPrototype' routine
	 */
	ThreadContext(ContextPrototype const & prototype) : gfal2_handle(0), infosys(prototype.infosys) {}

	/**
	 * Constructs a ThreadContext based on a prototype
	 *
	 * This is the actual constructor that creates gfal2_context!
	 *
	 * @param prototype : a ThreadContext wrapper around a prototype object
	 */
	ThreadContext(ThreadContext const & prototype);

	/**
	 * Destructor
	 *
	 * frees gfal2_context if it exits
	 */
	virtual ~ThreadContext();

	/**
	 * The thread initialisation routine that should be passed to ThreadPool
	 *
	 * @param ctx : thread context that should be initialized
	 */
	static void initContext(boost::any & ctx)
	{
		// creates the context using copy constructor based on the prototype
		ctx = ThreadContext(prototype);
	}

	gfal2_context_t get() const
	{
		return gfal2_handle;
	}

private:

	/// a prototype object (based on this prototype all ThreadContexts are created)
	static ContextPrototype prototype;
	/// gfal2 context of the given thread
	gfal2_context_t gfal2_handle;
	/// infosys name
	std::string infosys;
};

#endif /* ThreadContext_H_ */
