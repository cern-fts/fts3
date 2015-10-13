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
 *  GLite Utilities
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: glite-util.h,v 1.8 2006/08/22 08:29:29 badino Exp $
 *  Release: $Name:  $
 *
 *
 */

#ifndef GLITE_UTIL_H
#define GLITE_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Common URI schemes. */
#define GLITE_URI_LFN           "lfn"
#define GLITE_URI_GUID          "guid"
#define GLITE_URI_SRM           "srm"
#define GLITE_URI_HTTP          "http"
#define GLITE_URI_HTTPS         "https"
#define GLITE_URI_HTTPG         "httpg"

/*
 * Names of the environment variables that define the location of gLite
 * components.
 */
#define GLITE_LOCATION          "GLITE_LOCATION"
#define GLITE_LOCATION_VAR      "GLITE_LOCATION_VAR"
#define GLITE_LOCATION_LOG      "GLITE_LOCATION_LOG"
#define GLITE_TMP           "GLITE_TMP"

#include "ServiceDiscoveryIfce.h"

/**
 * Broken up URI
 */
typedef struct _glite_uri           glite_uri;

/**
 * \brief The broken up URI structure.
 *
 * The following combinations are valid:
 *  - hierarchical:
 *    - &lt;scheme&gt;://&lt;path&gt;
 *    - Non-LFN URIs: &lt;scheme&gt;://[&lt;endpoint&gt;]&lt;path&gt;[?&lt;query&gt;]
 *    - LFN URIs: &lt;scheme&gt;://&lt;endpoint&gt;?lfn=&lt;path&gt;
 *  - non-hierarchical: &lt;scheme&gt;:&lt;path&gt;
 */
struct _glite_uri
{
    int             hierarchical; /**< specifies wheter the URI is hierarhical or not. */
    char                *scheme; /**< the URI scheme */
    char                *endpoint; /**< the authority */
    char                *path; /**< the path part of the URI */
    char                *query; /**< the query part of the URI */
};

/**
 * \defgroup glite_util GLITE Utils
 * @{
 */

/**
 * \brief Look up a service with a given type using Service Discovery, using the
 * version as hint.
 *
 * Checks wheter an appropriate service exists, and gives back its name. In case
 * multiple results are returned, select the one with the closest match with
 * the given interface version. The rule applied to find the closest is:
 * 1. same version
 * 2. same major version number, greater minor
 * 3. same major and minor, greater patch
 * 4.(fallback) the first result returned.
 * @param type [IN] the type we are interested in.
 * @param name [IN] the name of the service, or the service site or host name.
 * @param version [IN] the service interface version hint.
 * @param error [OUT] an array for the error messages.
 *
 * @return
 */
char *glite_discover_service_by_version(const char *type, const char *name, const char * version, char **error);

/**
 * \brief Look up a service endpoint.
 *
 * Checks wheter an appropriate service exists, and gives back its endpoint.
 * @param type [IN] the type we are interested in.
 * @param name [IN] the name of the service.
 * @param error [OUT] an array for the error messages.
 *
 * @return
 */
char *glite_discover_endpoint(const char *type, const char *name, char **error);

/**
 * \brief Select the entry from a list using the version as hint.
 *
 * Select the entry that best matches the given version. If no match is found, the one with the
 * same major number and greater minor number is returned. As fallback solution, the first
 * entry is returned.
 *
 * @return
 */
const SDService * select_service_by_version(const SDServiceList * list, const char * version);

/**
 * \brief Returns the client VO list taken from a VOMS proxy.
 *
 * Checks the user's VOMS certificate (if available) and retrieves the list of VOs of the client.
 *
 * @return
 */
SDVOList *check_voms_proxy(void);

/**
 * \brief Returns the client VO list taken from an environment variable.
 *
 * Checks the environment variable SD_VO taking the list of VOs from it.
 *
 * @return
 */
SDVOList *check_vo_env(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* GLITE_UTIL_H */
