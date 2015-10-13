/*
 * Copyright (c) Members of the EGEE Collaboration. 2004.
 * See http://www.eu-egee.org/partners/ for details on the copyright holders.
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
 *
 *  GLite Data Management - Simple data delegation API
 *
 *  Authors:
 *      Zoltan Farkas <Zoltan.Farkas@cern.ch>
 *      Akos Frohner <Akos.Frohner@cern.ch>
 *
 */

#ifndef GLITE_DATA_DELEGATION_H
#define GLITE_DATA_DELEGATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

/**********************************************************************
 * Types
 */
/*
 * Opaque data structure used by the library.
 */
typedef struct _glite_delegation_ctx        glite_delegation_ctx;

/**********************************************************************
 * Constants
 */

/* Service type for Service Discovery */
#define GLITE_DELEGATION_SD_TYPE       "org.glite.Delegation"

/* Environment variable to override service type */
#define GLITE_DELEGATION_SD_ENV        "GLITE_SD_DELEGATION_TYPE"


/**********************************************************************
 * Function prototypes - library management functions
 */

/**
 * \brief Allocates a new delegation context.
 *
 * The context can be used to access only one service.
 *
 * If the endpoint is not a URL (HTTP, HTTPS or HTTPG), then
 * service discovery is used to locate the real service endpoint.
 * In this case the 'endpoint' parameter is the input of the
 * service discovery.
 *
 * This mechanism is executed upon the first service specific usage
 * of the context. After that point methods using the same context,
 * from any other interface will return an error.
 *
 * @param endpoint The name of the endpoint. Either an URL or a
 *  service name.
 * @param proxy Path to the local proxy
 * @return the context or NULL if memory allocation has failed.
 */
glite_delegation_ctx *glite_delegation_new(const char *endpoint, const char *proxy);

/**
 * Destroys a fireman context.
 *
 * @param ctx The global context.
 */
void glite_delegation_free(glite_delegation_ctx *ctx);

/**
 * \brief Gets the error message for the last failed operation.
 *
 * The returned pointer is valid only till the next call to any of
 * the library's functions with the same context pointer.
 *
 * @param ctx The global context.
 * @return the error message.
 */
char *glite_delegation_get_error(glite_delegation_ctx *ctx);

/**
 * \brief Delegate a local credential to the remote service.
 *
 * If forced, then it will try to renew an existing delegated
 * credential.
 *
 * @param ctx The global context.
 * @param delegationID The id used to identify the delegation process and the
 *                     resulting delegated credentials in the server.
 * @param expiration Desired expritation time in minutes.
 * @param force Force the delegation, even if there is a valid credential.
 *
 * @return 0 if OK, -1 in case of error
 */
int glite_delegation_delegate(glite_delegation_ctx *ctx,
                              const char *delegationID,
                              int expiration, int force);

/**
 * Get the expiration time.
 *
 * @param ctx The global context.
 * @param delegationID The id used to identify the delegation process and the
 *                     resulting delegated credentials in the server.
 * @param expiration Expritation time.
 *
 * @return 0 if OK, -1 in case of error
 */
int glite_delegation_info(glite_delegation_ctx *ctx,
                          const char *delegationID,
                          time_t *expiration);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* GLITE_DATA_SECURITY_DELEGATION_H */
