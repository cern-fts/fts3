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

#ifndef INSTANCEHOLDER_H_
#define INSTANCEHOLDER_H_

#include <memory>

namespace fts3
{
namespace common
{


/**
 * The InstanceHolder carries out the task of holding and grating access to an instance.
 * 	The instance type is specified by the template parameter.
 *
 * The InstanceHolder can be used for example as the base class in order to implement
 * singleton design pattern. In this case, the deriving class should give itself as the
 * template parameter. Since the constructor of the derived class should be private, the
 * class should be a friend of InstanceHolder. For example:
 *
 * 		class C : public InstanceHolder<C> {
 *
 * 			friend class InstanceHolder<C>;
 *
 * 			// ...
 * 		}
 *
 */
template <typename T>
class InstanceHolder
{

public:

    /**
     * Gets a references to T. Note that the method is not thread safe.
     * If a thread-safe implementation is needed, the one provided by
     * ThreadSafeInstanceHolder should be used.
     *
     * @return reference to T
     *
     * @see ThreadSafeInstanceHolder
     */
    static T& getInstance()
    {
        // thread safe lazy loading
        if (instance.get() == 0)
            {
                instance.reset(new T);
            }
        return *instance;
    }


    /**
     * Destructor (empty).
     */
    virtual ~InstanceHolder() {};

protected:

    /**
     * Default constructor (empty).
     *
     * Private
     */
    InstanceHolder() {};

    /**
     * The single instance
     */
    static std::unique_ptr<T> instance;

private:
    /**
     * Copying constructor.
     *
     * Private, should not be used
     */
    InstanceHolder(InstanceHolder const&);

    /**
     * Assignment operator.
     *
     * Private, should not be used
     */
    InstanceHolder& operator= (InstanceHolder const&);
};

// initialize single instance pointer with 0
template<typename T>
std::unique_ptr<T> InstanceHolder<T>::instance;

}
}

#endif /* INSTANCEHOLDER_H_ */
